"""Fail-closed check of the actual emitted readelf report, not a support claim."""
import json
import re
import sys
from pathlib import Path

def issues(report):
    failures=[]
    for label,pattern in [('ELF32',r'Class:\s+ELF32'),('little-endian',r'Data:.*little endian'),
                          ('ARM',r'Machine:\s+ARM\b'),('hard-float',r'Flags:.*hard-float ABI'),
                          ('shared-object',r'Type:\s+DYN\b'),
                          ('eager binding',r'\(BIND_NOW\)|\(FLAGS(?:_1)?\)[^\n]*\bNOW\b'),
                          ('resident lifetime',r'\(FLAGS_1\)[^\n]*\bNODELETE\b')]:
        if not re.search(pattern,report):failures.append('missing '+label)
    needed=re.findall(r'\(NEEDED\).*\[([^\]]+)\]',report)
    for name in ['Qt6Core','Qt6Gui','Qt6Network','Qt6Qml','Qt6Quick']:
        if not any(n.startswith('lib'+name+'.so.') for n in needed):failures.append('missing dependency '+name)
    # Check both exported/undefined symbols and dynamic dependency names.
    for name in ['Dobby','reader_insertion_hooks','reader_worker_native','ResidentMutation','ResidentSelection','qt_qml_creation_bridge']:
        if name in report:failures.append('forbidden native dependency '+name)
    if re.search(r'\(TEXTREL\)|\(FLAGS\)[^\n]*\bTEXTREL\b',report):failures.append('text relocations')
    return failures

if __name__=='__main__':
    result=issues(Path(sys.argv[1]).read_text())
    print(json.dumps({'valid':not result,'issues':result}))
    raise SystemExit(bool(result))
