// Actual local owned descriptors/files only. Reuse the unchanged original helpers.
#define main captureFilesOriginalMain
#include "reader_capture_files_fixture.cpp"
#undef main
#include <cstdio>

namespace {
std::vector<int> owned(const Root &root){
    struct stat directory{},pending{},final{};check(::fstat(root.fd,&directory)==0,"borrowed directory stat");
    const bool hasPending=root.stat(".pending-image",pending),hasFinal=root.stat("visual-image.png",final);
    std::vector<int> result;
    for(int fd=0;fd<1024;++fd){
        if(fd==root.fd)continue;
        struct stat current{};if(::fstat(fd,&current))continue;
        const auto matches=[&](const struct stat &s){return current.st_dev==s.st_dev&&current.st_ino==s.st_ino;};
        if(matches(directory)||(hasPending&&matches(pending))||(hasFinal&&matches(final)))result.push_back(fd);
    }
    return result;
}
void alive(const std::vector<int> &fds){for(int fd:fds)check(::fcntl(fd,F_GETFD)>=0,"active operation retains every saved descriptor");}
void closed(const std::vector<int> &fds){for(int fd:fds){errno=0;check(::fcntl(fd,F_GETFD)==-1&&errno==EBADF,"owned descriptor checked closed");}}
void cleanup(const std::shared_ptr<Files> &files,Root &root,std::size_t count){
    const auto descriptors=owned(root);check(descriptors.size()==count,"exact owned FD shape before retirement");
    const auto r=files->retire();check(r.ok&&r.attempted&&!r.linked&&!r.committed&&r.bytes==0&&r.refusal.isEmpty(),"cleanup success is not a file commit");
    closed(descriptors);check(files->imageFd()==-1&&::fcntl(root.fd,F_GETFD)>=0,"borrowed caller root survives cleanup");
    check(owned(root).empty(),"no helper image/pin/root descriptors remain");
}
struct OnDestroy {
    std::weak_ptr<Files> files;std::vector<int> descriptors;
    bool armed=false,fired=false,nestedRefused=false;
};
struct RetireAtUnwind {
    std::shared_ptr<OnDestroy> state;
    bool operator()()const{state->armed=true;return true;}
    ~RetireAtUnwind(){
        if(!state||!state->armed||state->fired)return;
        state->fired=true;const auto files=state->files.lock();
        if(!files)return;
        const int image=files->imageFd();
        const auto r=files->retire();state->nestedRefused=!r.ok&&!r.attempted&&r.refusal=="retire-reentered"&&image>=0&&::fcntl(image,F_GETFD)>=0;
        for(int fd:state->descriptors)if(::fcntl(fd,F_GETFD)<0)state->nestedRefused=false;
    }
};
}
int main(){try{
    scenario("retire-before-open",[]{
        Root root;const auto files=Files::create(root.fd,yes);check(bool(files),"root admitted");cleanup(files,root,1);
        const auto again=files->retire();check(!again.ok&&!again.attempted&&again.refusal=="retire-spent","one cleanup attempt only");
        unsigned permissions=0;const Permit unexpected=[&]{++permissions;return true;};
        refused(files->openImage(unexpected));refused(files->sealImage(1,unexpected));refused(files->commitImage(unexpected));
        refused(files->publishScalar(scalar,ScalarKind::Diagnostic,unexpected));
        check(!permissions&&!root.exists(".pending-image")&&!root.exists(".pending-result")&&!root.exists("selected-result"),"no late operations, permissions or new files");
    });
    scenario("retire-open-writer",[]{
        Root root;const auto files=opened(root);writeImage(files);struct stat before{},after{};check(root.stat(".pending-image",before),"pending image captured");
        cleanup(files,root,2);check(root.stat(".pending-image",after)&&detail::same(before,after)&&root.read(".pending-image")==image,"pending image preserved byte/metadata exact");
    });
    scenario("retire-sealed-pin",[]{
        Root root;const auto files=opened(root);sealed(files);const auto fds=owned(root);unsigned pins=0;
        for(int fd:fds)if((::fcntl(fd,F_GETFL)&O_PATH)==O_PATH)++pins;
        check(pins==1&&files->imageFd()==-1,"sealed image has only nonwritable inode pin");
        cleanup(files,root,2);check(root.read(".pending-image")==image&&!root.exists("visual-image.png"),"pin close never unlinks pending file");
    });
    scenario("retire-complete-pair",[]{
        Root root;const auto files=opened(root);sealed(files);committed(files->commitImage(yes),image.size());committed(files->publishScalar(scalar,ScalarKind::AcceptedPair,yes),scalar.size());
        struct stat a{},b{},againA{},againB{};check(root.stat("visual-image.png",a)&&root.stat("selected-result",b),"accepted pair stat");
        cleanup(files,root,2);check(root.stat("visual-image.png",againA)&&root.stat("selected-result",againB)&&detail::same(a,againA)&&detail::same(b,againB),"retirement preserves complete pair metadata");
        check(root.read("visual-image.png")==image&&root.read("selected-result")==scalar,"complete original pair preserved");
    });
    scenario("retire-diagnostic",[]{
        Root root;const auto files=Files::create(root.fd,yes);check(bool(files),"root admitted");committed(files->publishScalar(scalar,ScalarKind::Diagnostic,yes),scalar.size());
        cleanup(files,root,1);check(root.read("selected-result")==scalar&&!root.exists("visual-image.png"),"diagnostic bytes preserved without image");
    });
    for(bool scalarLink:{false,true})scenario(scalarLink?"retire-result-hardlinks":"retire-image-hardlinks",[=]{
        Root root;const auto files=scalarLink?Files::create(root.fd,yes):opened(root);check(bool(files),"root admitted");if(!scalarLink)sealed(files);
        const char *pending=scalarLink?".pending-result":".pending-image",*final=scalarLink?"selected-result":"visual-image.png";
        const Permit loseAfterLink=[&]{return !root.exists(final);};
        const auto r=scalarLink?files->publishScalar(scalar,ScalarKind::Diagnostic,loseAfterLink):files->commitImage(loseAfterLink);
        check(!r.ok&&r.linked&&!r.committed,"post-link permission loss preserved");
        struct stat p{},f{},pa{},fa{};check(root.stat(pending,p)&&root.stat(final,f)&&detail::same(p,f)&&p.st_nlink==2,"real linked forensic pair");
        cleanup(files,root,scalarLink?1:2);
        check(root.stat(pending,pa)&&root.stat(final,fa)&&detail::same(p,pa)&&detail::same(f,fa)&&pa.st_nlink==2,"cleanup does not remove partial hardlinks");
        check(root.read(pending)==root.read(final),"both original forensic names remain");
    });
    for(int phase=0;phase<5;++phase)scenario("retire-active-permit",[=]{
        Root root;auto files=phase<1||phase==3?Files::create(root.fd,yes):opened(root);check(bool(files),"root admitted");
        if(phase==1)writeImage(files);
        if(phase==2||phase==4)sealed(files);
        if(phase==4)committed(files->commitImage(yes),image.size());
        const auto before=owned(root);bool nested=false;Result nestedResult;
        const Permit reentrant=[&]{if(!nested){nested=true;alive(before);nestedResult=files->retire();alive(before);}return true;};
        Result r;
        switch(phase){case 0:r=files->openImage(reentrant);break;case 1:r=files->sealImage(image.size(),reentrant);break;case 2:r=files->commitImage(reentrant);break;case 3:r=files->publishScalar(scalar,ScalarKind::Diagnostic,reentrant);break;default:r=files->publishScalar(scalar,ScalarKind::AcceptedPair,reentrant);}
        check(nested&&!nestedResult.ok&&!nestedResult.attempted&&nestedResult.refusal=="retire-reentered"&&!r.ok,"nested cleanup refuses and poisons active operation");
        // seal's local writer closes on refusal; other retained descriptors stay
        // until this independent cleanup call after the operation has unwound.
        cleanup(files,root,phase==1?1:before.size());
        check(!root.exists(".pending-result")&&!root.exists("selected-result"),"nested cleanup cannot publish scalar");
    });
    scenario("retire-final-facade-loss",[]{
        Root root;auto files=opened(root);writeImage(files);const auto before=owned(root);const std::weak_ptr<Files> weak=files;Files *raw=files.get();
        const auto r=raw->sealImage(image.size(),[&]{const auto retained=weak.lock();check(bool(retained),"operation local self pins facade");const auto nested=retained->retire();check(!nested.ok&&!nested.attempted,"nested cleanup refused");files.reset();alive(before);return true;});
        check(!r.ok&&weak.expired(),"last caller loss unwinds then releases helper");closed(before);
        check(::fcntl(root.fd,F_GETFD)>=0&&root.read(".pending-image")==image,"facade loss preserves caller root and forensic bytes");
    });
    scenario("retire-permit-capture-unwind",[]{
        Root root;const auto files=Files::create(root.fd,yes);check(bool(files),"root admitted");const auto before=owned(root);
        auto state=std::make_shared<OnDestroy>();state->files=files;state->descriptors=before;const Permit permit=RetireAtUnwind{state};
        const auto openedResult=files->openImage(permit);check(openedResult.ok&&state->fired&&state->nestedRefused,"copied permit target destructor cannot retire active scope");
        alive(before);refused(files->sealImage(1,yes)); // Sticky reentry poisons later admission.
        cleanup(files,root,1);check(root.exists(".pending-image")&&!root.exists("visual-image.png"),"destructor reentry has only preserved pending evidence");
    });
    scenario("retire-close-unknown-not-retried",[]{
        Root root;const auto files=opened(root);const int bad=files->imageFd();check(::close(bad)==0,"owned fault injection closes borrowed image FD without reuse");
        const auto r=files->retire();check(!r.ok&&r.attempted&&!r.committed&&r.refusal=="retire-close-unknown"&&files->imageFd()==-1,"close failure retains uncertainty and invalidates stored FDs");
        check(owned(root).empty()&&::fcntl(root.fd,F_GETFD)>=0,"other owned FDs closed while caller survives");
        const int sentinel=::fcntl(root.fd,F_DUPFD_CLOEXEC,bad);check(sentinel==bad,"retired descriptor number reused by independent sentinel");
        const auto again=files->retire();check(!again.ok&&!again.attempted&&again.refusal=="retire-spent"&&::fcntl(sentinel,F_GETFD)>=0,"uncertain close is never retried against reused FD");
        check(::close(sentinel)==0,"sentinel cleanup");
    });
    std::printf("PASS capture retire scenarios=%u checks=%u actual_owned_fd_files=true native_execution=false\n",scenarios,checks);return 0;
}catch(const std::exception &e){std::fprintf(stderr,"FAIL capture retire %s scenarios=%u checks=%u\n",e.what(),scenarios,checks);return 1;}}
