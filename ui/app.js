'use strict';
const $ = id => document.getElementById(id);
let state,previous=new Map(),sort={key:'id',direction:1},selected='',open=new Map();
const columns=[['id','Sub-skill'],['ownFloor','Own floor'],['effFloor','Eff. floor'],['current','Current'],['potential','Potential'],['peak','Peak'],['U','Idle U (h)'],['decayPerDay','Decay / day'],['zone','Zone'],['worth','Worth'],['lastXP','Last XP']];
function send(action,args={}){if(typeof window.spAction==='function')window.spAction(JSON.stringify({action,...args}));else $('feedback').textContent=`Mock mode: ${action} ${JSON.stringify(args)}`;}
function node(tag,text){const n=document.createElement(tag);if(text!==undefined)n.textContent=text;return n;}
window.spUpdate=function(payload){try{state=typeof payload==='string'?JSON.parse(payload):payload;render();}catch(e){console.error(e);$('summary').textContent='Invalid snapshot: '+e.message;}};
function render(){
 if(!state)return;
 document.querySelectorAll('details[data-av]').forEach(d=>open.set(d.dataset.av,d.open));
 const d=state.debug||{};
 $('summary').textContent=`${state.state} · active ${(state.activeHours||0).toFixed(3)} h · XP ×${d.xp_mult} · time ×${d.time_scale} · no grace ${!!d.no_grace} · fixed XP ${!!d.fixed_xp} · write main ${!!d.write_main_skills}`;
 const tables=$('tables');tables.replaceChildren();const ids=[];const next=new Map();
 for(const skill of state.skills||[]){const details=node('details');details.dataset.av=skill.av;details.open=open.get(skill.av)??true;details.append(node('summary',`${skill.name} ${skill.F.toFixed(2)} (written ${skill.written})${skill.pending?' · pending':''}`));const line=node('p',`Base ${skill.base.toFixed(2)} · Rust ${skill.R.toFixed(2)} · Idle ${skill.Umain.toFixed(2)} h · Rust/day ${skill.rustPerDay.toFixed(2)} · External ${skill.external}`);line.className='mainline';details.append(line);
 const table=node('table'),head=node('thead'),tr=node('tr');for(const [key,title] of columns){const th=node('th',title);th.tabIndex=0;th.onclick=()=>{sort={key,direction:sort.key===key?-sort.direction:1};render();};th.onkeydown=e=>{if(e.key==='Enter')th.click();};tr.append(th);}head.append(tr);table.append(head);const body=node('tbody');
 for(const sub of [...skill.subs].sort((a,b)=>sort.direction*(typeof a[sort.key]==='string'?a[sort.key].localeCompare(b[sort.key]):a[sort.key]-b[sort.key]))){ids.push(sub.id);for(const [key] of columns)next.set(sub.id+'/'+key,sub[key]);if(!sub.id.toLowerCase().includes($('filter').value.toLowerCase()))continue;if($('untouched').checked&&sub.current===0&&sub.ownFloor===0&&sub.potential===0)continue;
 const row=node('tr');for(const [key] of columns){const value=sub[key],cell=node('td',typeof value==='number'?value.toFixed(key==='worth'?2:1):value);const old=previous.get(sub.id+'/'+key);if(old!==undefined&&old!==value){cell.className='changed';setTimeout(()=>cell.classList.remove('changed'),1000);}row.append(cell);}body.append(row);}
 table.append(body);const scroll=node('div');scroll.className='scroll';scroll.append(table);details.append(scroll);tables.append(details);}
 previous=next;selected=$('sub').value||selected;$('sub').replaceChildren(...ids.map(id=>{const o=node('option',id);o.value=id;return o;}));if(ids.includes(selected))$('sub').value=selected;
 $('equipped').textContent=(state.equipped||[]).map(x=>`${x.hand}: ${x.name||x.kind}\n  class=${x.class||''} sub=${x.sub||''} form=${x.form||''} direction=${x.direction||''} projectile=${x.projectile||''}\n  form level=${x.formLevel||0} sigma=${x.sigma||0}° sway=${x.sway||0}°`).join('\n');
}
document.querySelectorAll('[data-tab]').forEach(b=>b.onclick=()=>{for(const id of ['skills','debug'])$(id).hidden=id!==b.dataset.tab;});
document.querySelectorAll('[data-action]').forEach(b=>b.onclick=()=>send(b.dataset.action));
$('filter').oninput=render;$('untouched').onchange=render;$('close').onclick=()=>send('close');
$('reinit').onclick=()=>{if($('sure').checked){send('reinitAll');$('sure').checked=false;}};
$('give').onclick=()=>send('giveXP',{id:$('sub').value,raw:Number($('raw').value)});
$('set').onclick=()=>send('setCurrent',{id:$('sub').value,level:Number($('level').value)});
document.addEventListener('keydown',e=>{if(e.key==='Escape'||e.key==='F7'){e.preventDefault();send('close');}});
if(typeof window.spAction!=='function')fetch('mock.json').then(r=>r.json()).then(window.spUpdate).catch(e=>{$('summary').textContent='Serve ui/ over HTTP to load mock.json: '+e.message;});
