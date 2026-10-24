"""Prepare portable owned startup-admission inputs; never executes them."""
from pathlib import Path
import argparse,hashlib,json
def extract(startup,entry):
 helper=entry[entry.index('inline bool decimal('):entry.index('\nclass Entry final')]
 helper=helper.replace('inline QByteArray selfIdentity()','inline QByteArray realSelfIdentity()')
 a=startup.index('bool readPrivate(');b=startup.index('bool executableAdmission()',a)
 functions=startup[a:b]
 controller=startup.index('class Controller')
 fixed='        const auto selfIdentity=reader_selected::selfIdentity();'
 a=startup.index(fixed,controller)if fixed in startup[controller:]else startup.index("        const auto parts=window.chopped(1).split",controller)
 b=startup.index('        if(parts.size()!=5',a)
 return ('namespace reader_selected {\n'+helper+'\ninline QByteArray selfIdentity(){return identityFault?identityBytes:realSelfIdentity();}\n}\n'+functions+'\nbool controllerIdentityFragment(const QByteArray &window){\n'+startup[a:b]+'return parts.size()==5&&identity.size()==2;\n}\n')
def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('--out',type=Path,required=True);a.add_argument('--old-startup',type=Path);args=a.parse_args()
 here=Path(__file__).resolve().parent;args.out.mkdir(parents=True,exist_ok=False)
 sha=lambda b:hashlib.sha256(b).hexdigest();hashes={}
 for name in ('reader_selected_view_entry.h','reader_capture_files.h','reader_write_reopen_admission_fixture.cpp','reader_write_reopen_admission_test.sh'):
  raw=(here/name).read_bytes().replace(b'\r\n',b'\n');(args.out/name).write_bytes(raw);hashes[name]=sha(raw)
 entry=(args.out/'reader_selected_view_entry.h').read_text(encoding='utf8')
 versions={'fixed':here/'reader_write_reopen_startup.cpp'}
 if args.old_startup:versions['old']=args.old_startup
 for version,path in versions.items():
  raw=path.read_bytes().replace(b'\r\n',b'\n');text=raw.decode('utf-8-sig');(args.out/('startup-'+version+'.cpp')).write_bytes(raw)
  body=extract(text,entry).encode();folder=args.out/version;folder.mkdir();(folder/'admission.inc').write_bytes(body)
  hashes['startup-'+version+'.cpp']=sha(raw);hashes[version+'/admission.inc']=sha(body)
 result=dict(prepared_only=True,no_execution=True,native_acquisition_modeled=False,scope='actual extracted startup admission and identity fragment, owned root, explicit identity fault seam; no constructor or UI startup',versions=list(versions),sha256=hashes)
 (args.out/'inputs.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf8',newline='\n');print(json.dumps(result))
if __name__=='__main__':main()

