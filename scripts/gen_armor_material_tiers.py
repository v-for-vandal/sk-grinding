# Rows of refs/lorerim/armor-material-tiers.md from the research dump (armor-materials.tsv, materials.md).
import csv,sys,re,collections
rows=list(csv.DictReader(open(sys.argv[1],encoding='utf-8'),delimiter='\t'))
mat={}  # from materials.md: formkey -> smithing proposal
for l in open(sys.argv[2],encoding='utf-8'):
    c=[x.strip() for x in l.split('|')]
    if len(c)>9 and c[1].isdigit():
        fk=c[4].strip('`'); mat[fk]=c[8]
kwre=re.compile(r'([^;\[]+?) \[([0-9A-F]{6}:[^\]]+)\]')
agg=collections.defaultdict(lambda:{'types':collections.Counter(),'n':0,'names':collections.Counter()})
for r in rows:
    if r['type'] not in('HeavyArmor','LightArmor'): continue
    if 'Body' not in [s.strip() for s in r['slots'].split(',')]: continue
    for name,fk in kwre.findall(r['keywords']):
        name=name.strip()
        if not re.search(r'ArmorSet|ArmorMaterial',name): continue
        a=agg[(name,fk)]; a['types'][r['type']]+=1; a['n']+=1
        nm=r['name']
        if r['template']=='Null' and nm and ' of ' not in nm: a['names'][nm]+=1
out=[]
for (name,fk),a in sorted(agg.items(),key=lambda kv:-kv[1]['n']):
    t=', '.join(f"{k.replace('Armor','')} {v}" for k,v in a['types'].most_common())
    ex=', '.join(n for n,_ in a['names'].most_common(3)) or '—'
    out.append(f"| `{name}` | `{fk}` | {t} | {ex} | {mat.get(fk,'')} |  |  |")
print('\n'.join(out)); print(len(out),file=sys.stderr)
