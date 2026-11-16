#define COMPOSER_WRITER_TESTING
#include "composer_native_action.h"
#include "composer_anchor_walk.h"
#include "composer_plain_edit.h"
#include "composer_native_insert.h"
#include "composer_owned_edit.h"
#include <QGuiApplication>
#include <QTextCursor>
#include <cassert>
#include <climits>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
#include <thread>
#include <new>

namespace remarkable::composer_scene::testing {
struct Access {
    static std::unique_ptr<Profile> providers(bool scalars) {
        return std::unique_ptr<Profile>(new Profile(scalars));
    }
};
}
namespace remarkable::composer_writer::testing {
struct Access {
    static void transactions();
    static inline std::array<unsigned char,2> caches{1,0};
    static composer_scene::detail::CacheBytes readCaches(const composer_scene::Join &) { return caches; }
    static inline int makes = 0, submits = 0, releases = 0;
    static inline int mode = 0;
    static void make(detail::ActionOwner *out, void *item, int start, int length, int kind) {
        ++makes;
        assert(item == reinterpret_cast<void *>(0x1000) && start == 1 && length == 3 && kind == 0);
        if (mode == 1) throw std::runtime_error("factory before return");
        *out = {0x2010, 0x2000};
        if (mode == 2) throw std::runtime_error("partial result");
    }
    static bool matches(const detail::ActionOwner &owner, std::uintptr_t, int, int) {
        return mode != 3 && owner.object == 0x2010 && owner.control == 0x2000;
    }
    static void submit(void *stack, const detail::ActionOwner *owner) {
        ++submits;
        assert(stack == reinterpret_cast<void *>(0x3000) && owner->object == 0x2010);
        if (mode == 4) throw std::runtime_error("unknown native submission");
    }
    static void release(void *control) {
        ++releases; assert(control == reinterpret_cast<void *>(0x2000));
        if (mode == 5) throw std::runtime_error("unknown decrement");
    }
    static void makeInsert(detail::ActionOwner *out, void *item, int start, QString *text,
            const std::uint32_t *format) {
        ++makes;
        assert(item == reinterpret_cast<void *>(0x1000) && start == 1 && *text == QStringLiteral("\n\n"));
        assert(format && format[0] == 0 && format[1] == 0);
        if (mode == 1) throw std::runtime_error("factory before return");
        *out = {0x2010,0x2000};
        if (mode == 2) throw std::runtime_error("partial result");
        if (mode == 7) *text = QStringLiteral("normalized unexpectedly");
    }
    static bool matchesInsert(const detail::ActionOwner &owner, std::uintptr_t, int, const QString &) {
        return mode != 3 && owner.object == 0x2010 && owner.control == 0x2000;
    }
    static detail::InsertedInterval readIds(const detail::ActionOwner &) {
        return {{101,0x70000},{mode == 6 ? 103U : 102U,0x70000}};
    }
    static void insert() {
        for (mode = 0; mode != 8; ++mode) {
            makes = submits = releases = 0;
            detail::InsertProfile profile(makeInsert,submit,release,matchesInsert,readIds);
            detail::ActionReceipt receipt;
            {
                detail::Insertion insertion(profile);
                assert(!insertion.prepare(0x1000,1,{}));
                assert(!insertion.prepare(0x1000,1,QStringLiteral("\r")));
                assert(!insertion.prepare(0x1000,1,QString(QChar(0xd800))));
                assert(!insertion.prepare(0x1000,INT_MAX,QStringLiteral("\n\n")));
                assert(makes == 0);
                const bool prepared = insertion.prepare(0x1000,1,QStringLiteral("\n\n"));
                assert(prepared == (mode == 0 || mode == 4 || mode == 5 || mode == 6));
                assert(!insertion.interval());
                insertion.submit(0x3000); insertion.submit(0x3000);
                assert(bool(insertion.interval()) == (mode == 0 || mode == 5));
                insertion.finish(); insertion.finish();
                assert(!insertion.interval());
                receipt = insertion.receipt();
            }
            const bool prepared = mode == 0 || mode == 4 || mode == 5 || mode == 6;
            assert(makes == 1 && submits == int(prepared) && releases == int(prepared));
            assert(receipt.submissionAttempted == prepared);
            assert(receipt.submissionReturned == (prepared && mode != 4));
            assert(receipt.quarantined == (mode == 2 || mode == 3 || mode == 5 || mode == 7));
        }
        mode = 0;
        assert(!detail::InsertProfile::load());
        std::cout << "PASS insertion preparation, exact optional ABI, interval and unknown ownership\n";
    }
    static void run() {
        for (mode = 0; mode != 6; ++mode) {
            makes = submits = releases = 0;
            detail::ActionProfile profile(make, submit, release, matches);
            detail::ActionReceipt receipt;
            {
                detail::Removal action(profile);
                assert(!action.prepare(0, 1, 3));
                assert(!action.prepare(0x1000, -1, 3));
                assert(!action.prepare(0x1000, INT_MAX, 3));
                assert(makes == 0);
                const bool prepared = action.prepare(0x1000, 1, 3);
                assert(prepared == (mode == 0 || mode == 4 || mode == 5));
                assert(!action.prepare(0x1000, 1, 3));
                action.submit(0);
                assert(submits == 0);
                action.submit(0x3000);
                action.submit(0x3000);
                action.finish(); action.finish();
                receipt = action.receipt();
            }
            assert(makes == 1);
            const bool prepared = mode == 0 || mode == 4 || mode == 5;
            assert(submits == int(prepared) && releases == int(prepared));
            assert(receipt.constructionAttempted && receipt.constructed == prepared);
            assert(receipt.submissionAttempted == prepared);
            assert(receipt.submissionReturned == (prepared && mode != 4));
            assert(receipt.releaseAttempted == prepared);
            assert(receipt.releaseReturned == (prepared && mode != 5));
            assert(receipt.quarantined == (mode == 2 || mode == 3 || mode == 5));
        }
        mode = 0; makes = submits = releases = 0;
        detail::ActionProfile profile(make, submit, release, matches);
        { detail::Removal action(profile); assert(action.prepare(0x1000, 1, 3)); }
        assert(makes == 1 && submits == 0 && releases == 1);
        // Production profile is not available in an owned fixture process.
        assert(!detail::ActionProfile::load());
        std::cout << "PASS native action local ownership, refusal, unknown and no retry\n";
    }
    static void plain() {
        const auto baseOnly = composer_scene::testing::Access::providers(false);
        const auto withScalars = composer_scene::testing::Access::providers(true);
        assert(!detail::PlainProfile::acceptsProviders(*baseOnly));
        assert(detail::PlainProfile::acceptsProviders(*withScalars));
        QTextDocument document;
        document.setPlainText(QStringLiteral("outside\nowned draft\noutside"));
        auto *layout = document.documentLayout(); // Explicit fixture setup, not admission.
        auto *p = QTextDocumentPrivate::get(&document);
        QTextCursor cursor(&document); cursor.movePosition(QTextCursor::End);
        cursor.insertText(QStringLiteral("!"));
        std::uintptr_t vptr = 0, rtti = 0;
        std::memcpy(&vptr, layout, sizeof vptr);
        std::memcpy(&rtti, reinterpret_cast<const void *>(vptr - sizeof(void *)), sizeof rtti);
        detail::PlainProfile profile(vptr, rtti);
        profile.readCaches_ = readCaches; // Owned scalar provider; native offsets are not claimed on host Qt.
        composer_scene::Join joined;
        joined.document = &document; joined.root = 1; joined.control = 2; joined.participant = 3;
        joined.privateData = reinterpret_cast<std::uintptr_t>(p);
        joined.facts.documentAddress = reinterpret_cast<std::uintptr_t>(&document);
        std::atomic<bool> canceled{false};
        composer_text::detail::Guard guard{canceled,nullptr,composer_text::Clock::now() + std::chrono::seconds(5)};
        const auto before = document.toPlainText();
        const auto revision = document.revision();
        assert(profile.sample(joined,guard).admitted);
        assert(document.revision() == revision && document.toPlainText() == before);
        // A missing layout must stay missing; no public lazy getter is invoked.
        QTextDocument noLayout;
        auto absentJoin = joined;
        absentJoin.document = &noLayout;
        absentJoin.privateData = reinterpret_cast<std::uintptr_t>(QTextDocumentPrivate::get(&noLayout));
        absentJoin.facts.documentAddress = reinterpret_cast<std::uintptr_t>(&noLayout);
        assert(!QTextDocumentPrivate::get(&noLayout)->layout());
        assert(profile.sample(absentJoin,guard).failure == detail::PlainFailure::MissingLayout);
        assert(!QTextDocumentPrivate::get(&noLayout)->layout());
        detail::PlainProfile wrongType(vptr + sizeof(void *), rtti);
        assert(wrongType.sample(joined,guard).failure == detail::PlainFailure::LayoutType);
        detail::PlainProfile wrongRtti(vptr,rtti + 1);
        assert(wrongRtti.sample(joined,guard).failure == detail::PlainFailure::LayoutType);
        layout->setParent(nullptr);
        assert(profile.sample(joined,guard).failure == detail::PlainFailure::LayoutOwner);
        layout->setParent(&document);
        caches[0] = 0;
        assert(profile.sample(joined,guard).failure == detail::PlainFailure::UndoState);
        caches[0] = 1;
        caches[1] = 1;
        assert(profile.sample(joined,guard).failure == detail::PlainFailure::UndoState);
        caches[1] = 0;
        document.blockSignals(true);
        assert(profile.sample(joined,guard).failure == detail::PlainFailure::UndoState);
        document.blockSignals(false);
        cursor.beginEditBlock();
        assert(profile.sample(joined,guard).failure == detail::PlainFailure::Join);
        cursor.endEditBlock();
        std::thread other([&] {
            assert(profile.sample(joined,guard).failure == detail::PlainFailure::Thread);
        }); other.join();
        canceled = true;
        assert(profile.sample(joined,guard).failure == detail::PlainFailure::Canceled);
        canceled = false;
        // Rich content outside the proposed composer still refuses whole-doc
        // admission; no drawing/handler lookup is needed to discover it.
        cursor.movePosition(QTextCursor::Start);
        QTextCharFormat format; format.setObjectType(QTextFormat::UserObject);
        cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
        const auto rich = profile.sample(joined,guard);
        assert(!rich.admitted && rich.failure == detail::PlainFailure::Text &&
            rich.text.failure == composer_text::Failure::Structure);
        assert(!detail::PlainProfile::load());
        std::cout << "PASS existing layout, whole plain document, quiet undo and thread preflight\n";
    }
};
}
namespace {
using namespace remarkable;
using namespace composer_writer::detail;
struct Memory {
    std::map<std::uintptr_t, std::uint32_t> words;
    std::map<std::uintptr_t, std::uint16_t> units;
    mutable std::size_t wordReads = 0;
    std::uint32_t word(std::uintptr_t address, std::size_t offset) const {
        ++wordReads; return words.at(address + offset);
    }
    std::uint16_t unit(std::uintptr_t address, std::size_t at) const {
        return units.at(address + 2 * at);
    }
};
struct Fixture {
    static constexpr std::uintptr_t root = 0x1000, sentinel = root + 0x80;
    Memory memory;
    std::vector<std::uintptr_t> nodes;
    composer_text::Result projection;
    std::atomic<bool> canceled{false};
    composer_text::detail::Guard guard{canceled, nullptr, composer_text::Clock::now() + std::chrono::seconds(5)};
    Fixture() {
        projection.status = composer_text::Status::Copied;
        memory.words[sentinel] = sentinel; memory.words[sentinel + 4] = sentinel;
        projection.characterCount = 1;
    }
    void add(NativeId base, std::u16string text, std::uint32_t deleted = 0, unsigned variant = 0) {
        const auto address = 0x2000 + nodes.size() * 0x40;
        const auto data = 0x100000 + nodes.size() * 0x4000;
        const auto previous = nodes.empty() ? sentinel : nodes.back();
        memory.words[previous] = address;
        memory.words[address] = sentinel;
        memory.words[address + 4] = previous;
        memory.words[sentinel + 4] = address;
        memory.words[address + 8] = base.counterLow;
        memory.words[address + 12] = base.counterHighAuthor;
        memory.words[address + 0x24] = data;
        memory.words[address + 0x28] = text.size();
        memory.words[address + 0x2c] = variant;
        memory.words[address + 0x30] = deleted;
        for (std::size_t i = 0; i < text.size(); ++i) {
            memory.units[data + i * 2] = text[i];
            if (!deleted && projection.count < composer_text::Result::Capacity)
                projection.units[projection.count++] = text[i] == u'\n' ? 0x2029 : text[i];
        }
        projection.range.length = projection.count;
        projection.characterCount = static_cast<int>(projection.count + 1);
        nodes.push_back(address);
    }
    AnchorPositions walk(NativeId left = {101, 0x70000}, NativeId right = {105, 0x70000}) {
        return walkAnchors(root, left, right, projection, guard, memory);
    }
};
struct TransactionState {
    QTextDocument *document = nullptr;
    Fixture *native = nullptr;
    unsigned mode = 0;
    int position = 0, length = 0, insertCalls = 0, removeCalls = 0, releases = 0, joins = 0;
    bool removing = false, effected = false;
    std::shared_ptr<std::atomic<bool>> canceled;
};
thread_local TransactionState transaction;
composer_scene::Join transactionJoin(::Scene *) {
    ++transaction.joins;
    if (transaction.mode == 7 && transaction.removing && transaction.effected)
        throw std::runtime_error("post-effect join");
    composer_scene::Join joined;
    joined.document = transaction.document;
    joined.root = Fixture::root; joined.control = 2; joined.participant = 3;
    joined.privateData = reinterpret_cast<std::uintptr_t>(QTextDocumentPrivate::get(joined.document));
    joined.facts.documentAddress = reinterpret_cast<std::uintptr_t>(joined.document);
    return joined;
}
std::uintptr_t transactionContext(::Scene *,const composer_scene::Join &) { return 0x3000; }
AnchorPositions transactionAnchors(std::uintptr_t root,NativeId left,NativeId right,
        const composer_text::Result &text,const composer_text::detail::Guard &guard) {
    return walkAnchors(root,left,right,text,guard,transaction.native->memory);
}
AnchorPositions transactionSequence(std::uintptr_t root,const composer_text::Result &text,
        const composer_text::detail::Guard &guard) {
    return inspectSequence(root,text,guard,transaction.native->memory);
}
void resetNative(bool draft, bool deleted = false, bool changedOutside = false) {
    auto &f=*transaction.native;
    f.memory={};f.nodes.clear();f.projection={};f.projection.status=composer_text::Status::Copied;
    f.memory.words[Fixture::sentinel]=Fixture::sentinel;f.memory.words[Fixture::sentinel+4]=Fixture::sentinel;
    f.add({100,0x70000},u"outside!");
    f.add({1000,0x70000},u"\n");
    if(draft)f.add({2000,0x70000},u"draft");
    if(deleted)f.add({2000,0x70000},u"",5);
    f.add({1001,0x70000},u"\n");
    if(changedOutside)f.add({3000,0x70000},u"X");
}
void transactionMakeRemove(ActionOwner *out,void *root,int start,int length,int kind) {
    assert(root==reinterpret_cast<void *>(Fixture::root) && kind==0);
    transaction.position=start;transaction.length=length;transaction.removing=true;
    *out={0x2010,0x2000};
    if(transaction.mode==4) {
        QTextCursor cursor(transaction.document);cursor.movePosition(QTextCursor::End);cursor.insertText(QStringLiteral("X"));
        resetNative(true,false,true);
    }
    if(transaction.mode==10)transaction.canceled->store(true);
}
void transactionMakeInsert(ActionOwner *out,void *root,int start,QString *text,const std::uint32_t *flags) {
    assert(root==reinterpret_cast<void *>(Fixture::root) && *text==QStringLiteral("\n\n") && flags && !flags[1]);
    transaction.position=start;transaction.removing=false;*out={0x2010,0x2000};
}
bool transactionMatchesRemove(const ActionOwner &owner,std::uintptr_t,int,int) { return owner.object==0x2010; }
bool transactionMatchesInsert(const ActionOwner &owner,std::uintptr_t,int,const QString &) { return owner.object==0x2010; }
InsertedInterval transactionIds(const ActionOwner &) { return {{1000,0x70000},{1001,0x70000}}; }
void transactionSubmit(void *stack,const ActionOwner *owner) {
    assert(stack==reinterpret_cast<void *>(0x3000) && owner->object==0x2010);
    QTextCursor cursor(transaction.document);
    cursor.setPosition(transaction.position);
    if(transaction.removing) {
        ++transaction.removeCalls;
        cursor.setPosition(transaction.position+transaction.length,QTextCursor::KeepAnchor);
        cursor.removeSelectedText();transaction.effected=true;
        resetNative(false,true);
        if(transaction.mode==5)throw std::runtime_error("native effect followed by throw");
        if(transaction.mode==6) {
            cursor.movePosition(QTextCursor::End);cursor.insertText(QStringLiteral("X"));
            resetNative(false,true,true);
        }
        if(transaction.mode==11)transaction.canceled->store(true);
    } else {
        ++transaction.insertCalls;cursor.insertText(QStringLiteral("\n\n"));resetNative(false);
    }
}
void transactionRelease(void *control) { assert(control==reinterpret_cast<void *>(0x2000));++transaction.releases; }
void anchors() {
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz");
        const auto result = f.walk();
        assert(result.matched && result.left == 1 && result.right == 5 && result.nodes == 1);
        assert(result.draftCount==3 && result.draft[0]==u'h' && result.draft[2]==u'y');
        assert(f.walk({0,0x70000},{105,0x70000}).failure == AnchorFailure::InvalidId);
    }
    {
        Fixture f; f.add({0,0x90000}, u"unrelated");
        f.add({100,0x70000}, u"a\nhey\nz");
        assert(f.walk().failure == AnchorFailure::InvalidId);
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\n");
        f.add({900,0x90000}, u"", 1000);
        f.add({102,0x70000}, u"hey\nz");
        const auto result = f.walk();
        assert(result.matched && result.nodes == 3 && result.right == 5);
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz");
        f.add({101,0x70000}, u"", 1);
        const auto result=f.walk();
        assert(result.failure == AnchorFailure::Tombstone && result.draftCount==0);
        assert(std::all_of(result.draft.begin(),result.draft.end(),[](auto unit){return unit==0;}));
    }
    {
        Fixture f;f.add({100,0x70000},u"a\nhey\nthere\nz");
        const auto result=f.walk({101,0x70000},{111,0x70000});
        assert(result.matched && result.draftCount==9 && result.draft[3]==0x000a);
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz");
        f.add({101,0x70000}, u"\n");
        assert(f.walk().failure == AnchorFailure::Duplicate);
    }
    {
        Fixture f; f.add({100,0x80000}, u"a\nhey\nz");
        assert(f.walk().failure == AnchorFailure::Missing); // Same text, different native author.
    }
    {
        Fixture f; f.add({200,0x70000}, u"a\nhey\nz");
        assert(f.walk().failure == AnchorFailure::Missing); // Recreated-looking LF IDs differ.
    }
    {
        Fixture f; f.add({100,0x70000}, u"aXhey\nz");
        assert(f.walk().failure == AnchorFailure::NotLineFeed);
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz");
        assert(f.walk({105,0x70000},{101,0x70000}).failure == AnchorFailure::Order);
        f.projection.units[3] = u'X';
        assert(f.walk().failure == AnchorFailure::Projection);
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz", 0, 1);
        assert(f.walk().failure == AnchorFailure::Variant);
    }
    {
        Fixture f; f.add({0xffffffff,0x7ffff}, u"ab");
        assert(f.walk().failure == AnchorFailure::CounterOverflow);
    }
    {
        Fixture f; f.add({0xfffffffe,0x70000}, u"\nxy\n");
        assert(f.walk({0xfffffffe,0x70000},{1,0x70001}).matched); // Checked 32-bit carry.
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz");
        f.memory.words[f.nodes[0] + 4] = 0xdead;
        assert(f.walk().failure == AnchorFailure::Link);
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz");
        f.memory.words[Fixture::sentinel + 4] = Fixture::sentinel;
        assert(f.walk().failure == AnchorFailure::Link);
    }
    {
        Fixture f;
        for (unsigned i = 0; i < 1025; ++i) f.add({i + 1000,0x90000}, u"", 1);
        const auto result = f.walk();
        assert(result.failure == AnchorFailure::NodeLimit && result.nodes == 1024);
    }
    {
        Fixture f; f.add({100,0x70000}, std::u16string(4097, u'x'));
        assert(f.walk().failure == AnchorFailure::TextLimit);
    }
    {
        Fixture f; f.add({100,0x70000}, u"a\nhey\nz");
        f.canceled = true;
        assert(f.walk().failure == AnchorFailure::Canceled && f.memory.wordReads == 0);
        f.canceled = false; f.guard.deadline = composer_text::Clock::now();
        assert(f.walk().failure == AnchorFailure::Deadline && f.memory.wordReads == 0);
    }
    std::cout << "PASS bounded native anchor walk, tombstones, live IDs, UTF16 and projection\n";
}
}
void remarkable::composer_writer::testing::Access::transactions() {
    const auto gui=std::this_thread::get_id();
    {
        OwnedEdit guiDenied(1,{},{},{});
        std::atomic<bool> canceled{false};
        composer_text::detail::Guard guard{canceled,nullptr,composer_text::Clock::now()+std::chrono::seconds(1)};
        const auto denied=guiDenied.provision(reinterpret_cast<::Scene *>(1),guard,std::make_shared<std::atomic<bool>>(false));
        assert(denied.receipt.failure==EditFailure::Thread && !denied.receipt.action.constructionAttempted);
    }
    auto *worker=QThread::create([gui] {
        for(unsigned scenario=0;scenario!=14;++scenario) {
            QTextDocument document;
            document.setPlainText(QStringLiteral("outside"));
            auto *layout=document.documentLayout();
            QTextCursor cursor(&document);cursor.movePosition(QTextCursor::End);cursor.insertText(QStringLiteral("!"));
            Fixture memory;memory.add({100,0x70000},u"outside!");
            transaction={};transaction.document=&document;transaction.native=&memory;transaction.mode=scenario;
            transaction.canceled=std::make_shared<std::atomic<bool>>(false);
            std::uintptr_t vptr=0,rtti=0;
            std::memcpy(&vptr,layout,sizeof vptr);
            std::memcpy(&rtti,reinterpret_cast<const void *>(vptr-sizeof(void *)),sizeof rtti);
            auto plain=std::unique_ptr<detail::PlainProfile>(new detail::PlainProfile(vptr,rtti));
            plain->readCaches_=readCaches;
            auto remove=std::unique_ptr<detail::ActionProfile>(new detail::ActionProfile(
                transactionMakeRemove,transactionSubmit,transactionRelease,transactionMatchesRemove));
            auto insert=std::unique_ptr<detail::InsertProfile>(new detail::InsertProfile(
                transactionMakeInsert,transactionSubmit,transactionRelease,transactionMatchesInsert,transactionIds));
            OwnedEdit engine(scenario==13?1:reinterpret_cast<std::uintptr_t>(QThread::currentThread()),std::move(remove),std::move(insert),std::move(plain),gui);
            engine.join_=transactionJoin;engine.context_=transactionContext;engine.anchors_=transactionAnchors;
            engine.sequence_=transactionSequence;
            auto visit=std::make_shared<std::atomic<bool>>(false);
            composer_text::detail::Guard guard{*transaction.canceled,nullptr,composer_text::Clock::now()+std::chrono::seconds(5)};
            auto *scene=reinterpret_cast<::Scene *>(1); // Only the owned join sees this value.
            if(scenario==12)memory.memory.words[memory.nodes[0]+0x2c]=1;
            const auto issued=engine.provision(scene,guard,visit);
            if(scenario==13) {
                assert(issued.receipt.failure==EditFailure::Thread && transaction.joins==0 && transaction.insertCalls==0);continue;
            }
            if(scenario==12) {
                assert(issued.receipt.status==EditStatus::Refused && !issued.range && transaction.insertCalls==0);
                assert(document.toPlainText()==QStringLiteral("outside!"));continue;
            }
            assert(issued.receipt.status==EditStatus::ProjectionVerified && issued.range &&
                issued.receipt.action.submissionAttempted && issued.receipt.action.releaseReturned);
            assert(transaction.insertCalls==1 && transaction.removeCalls==0 && document.toPlainText()==QStringLiteral("outside!\n\n"));
            cursor.setPosition(9);cursor.insertText(QStringLiteral("draft"));resetNative(true);
            const auto revision=document.revision();
            const auto observed=engine.observe(scene,*issued.range,guard);
            assert(observed.observed && observed.text==QStringLiteral("draft") && observed.revision==revision);
            if(scenario==3)visit->store(true);
            if(scenario==8) {
                // Original LF is tombstoned; a recreated-looking LF has a new ID.
                memory.memory.words[memory.nodes[1]+0x30]=1;
                memory.add({9999,0x70000},u"\n");
            }
            if(scenario==9) {
                // Actual object destruction/reconstruction at the SAME address.
                // A QPointer to the old embedded incarnation must stay null.
                document.~QTextDocument();new (&document) QTextDocument;
                document.setPlainText(QStringLiteral("outside!\ndraft\n"));
                assert(!issued.range->document);
            }
            const auto result=engine.clear(scene,*issued.range,scenario==1?revision-1:revision,
                scenario==2?QStringLiteral("other"):QStringLiteral("draft"),guard);
            const bool attempted=scenario==0 || scenario==5 || scenario==6 || scenario==7 || scenario==11;
            assert(transaction.removeCalls==int(attempted));
            assert(result.action.submissionAttempted==attempted);
            if(scenario==0) {
                assert(result.status==EditStatus::ProjectionVerified && result.revisionsPresent);
                assert(document.toPlainText()==QStringLiteral("outside!\n\n"));
            } else if(attempted) {
                assert(result.status==EditStatus::Unknown && !result.revisionsPresent);
                assert(transaction.releases==2);
            } else {
                assert(result.status==EditStatus::Refused);
                assert(document.toPlainText().contains(QStringLiteral("draft")));
            }
            if(scenario==4 || scenario==10)assert(result.action.constructionAttempted && result.action.releaseReturned);
            assert(result.status!=EditStatus::ProjectionVerified || !visit->load());
        }
    });
    worker->start();assert(worker->wait(20000));delete worker;
    std::cout << "PASS private provision/readback and conditional clear, stale/unknown/draft controls\n";
}
int main(int argc, char **argv) {
    QGuiApplication application(argc, argv);
    remarkable::composer_writer::testing::Access::run();
    remarkable::composer_writer::testing::Access::insert();
    remarkable::composer_writer::testing::Access::plain();
    anchors();
    remarkable::composer_writer::testing::Access::transactions();
}
