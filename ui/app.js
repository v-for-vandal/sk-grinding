'use strict';
// Status panel (impl/status-ui.md). Default view: one bar per sub-skill. Debug mode (MCM →
// Skill Practice → Debug mode, snapshot field `debugMode`) adds the summary line, the Debug tab
// and the *Debug view* checkbox that swaps the bars for the full table.
const $ = id => document.getElementById(id);
let state,previous=new Map(),sort={key:'id',direction:1},selected='',open=new Map();
const columns=[['id','Sub-skill'],['ownFloor','Own floor'],['effFloor','Eff. floor'],['current','Current'],['potential','Potential'],['peak','Peak'],['U','Idle U (h)'],['decayPerDay','Decay / day'],['zone','Zone'],['worth','Worth'],['lastXP','Last XP']];
function send(action,args={}){if(typeof window.spAction==='function')window.spAction(JSON.stringify({action,...args}));else $('feedback').textContent=`Mock mode: ${action} ${JSON.stringify(args)}`;}
function node(tag,text,className){const n=document.createElement(tag);if(text!==undefined)n.textContent=text;if(className)n.className=className;return n;}
window.spUpdate=function(payload){try{state=typeof payload==='string'?JSON.parse(payload):payload;render();}catch(e){console.error(e);$('summary').textContent='Invalid snapshot: '+e.message;}};

// "destruction/missile" → "Missile", "1h-war-axe" → "War axe".
function label(id){const s=id.split('/').pop().replace(/^(1h|2h)-/,'').replace(/-/g,' ');return s.charAt(0).toUpperCase()+s.slice(1);}
function visible(sub){
 if(!sub.id.toLowerCase().includes($('filter').value.toLowerCase())&&!label(sub.id).toLowerCase().includes($('filter').value.toLowerCase()))return false;
 return !($('untouched').checked&&sub.current===0&&sub.ownFloor===0&&sub.potential===0);
}
function block(skill,title){
 const details=node('details');details.dataset.av=skill.av;details.open=open.get(skill.av)??true;
 details.append(node('summary',title));return details;
}

function render(){
 if(!state)return;
 document.querySelectorAll('details[data-av]').forEach(d=>open.set(d.dataset.av,d.open));
 const debug=!!state.debugMode;
 document.body.classList.toggle('debug',debug);
 if(!debug&&!$('debug').hidden)showTab('skills');
 const table=debug&&$('tableView').checked;
 $('bars').hidden=table;$('tables').hidden=!table;
 const d=state.debug||{};
 $('summary').textContent=!debug?(state.state==='pending'?'Sub-skills are not initialized yet.':''):`${state.state} · active ${(state.activeHours||0).toFixed(3)} h · XP ×${d.xp_mult} · time ×${d.time_scale} · no grace ${!!d.no_grace} · fixed XP ${!!d.fixed_xp} · write main ${!!d.write_main_skills}`;
 if(table){$('bars').replaceChildren();renderTables();}else{$('tables').replaceChildren();previous=new Map();renderBars();}
 const ids=(state.skills||[]).flatMap(s=>s.subs.map(x=>x.id));
 selected=$('sub').value||selected;$('sub').replaceChildren(...ids.map(id=>{const o=node('option',id);o.value=id;return o;}));if(ids.includes(selected))$('sub').value=selected;
 $('equipped').textContent=(state.equipped||[]).map(x=>`${x.hand}: ${x.name||x.kind}\n  class=${x.class||''} sub=${x.sub||''} form=${x.form||''} direction=${x.direction||''} projectile=${x.projectile||''}\n  form level=${x.formLevel||0} sigma=${x.sigma||0}° sway=${x.sway||0}°`).join('\n');
}

// Bar: [0, floor] colour 1, [floor, current] colour 2, [current, potential] colour 2 hatched,
// the rest empty. Each part has its own tooltip (our own element: Ultralight leaves native
// `title` tooltips to the host, PrismaUI may not draw them).
const tips={floor:'Drilled into the bones',current:'Learned',potential:'Been there once'};
// One scale for every bar: 100, or more when a value is above 100 (up to the hard cap).
function renderBars(){
 const subs=(state.skills||[]).flatMap(s=>s.subs);
 const top=Math.max(100,...subs.map(s=>Math.max(s.current,s.potential)));
 const scale=Math.ceil(top/25)*25,pct=v=>`${Math.max(0,Math.min(100,v/scale*100))}%`;
 const out=[];
 for(const skill of state.skills||[]){
  const details=block(skill,'');
  const head=details.firstChild,next=node('span',undefined,'skill-next'),fill=node('span');
  fill.style.width=`${(skill.toNext??0)*100}%`;next.append(fill);next.dataset.tip='Next level';next.dataset.value=`${Math.floor((skill.toNext??0)*100)} %`;
  head.replaceChildren(node('span',skill.name,'skill-name'),node('span',String(skill.written),'skill-level'),next);
  if(skill.pending)head.append(node('span','not initialized','skill-note'));
  const list=node('div',undefined,'bars');
  for(const sub of skill.subs){
   if(!visible(sub))continue;
   const floor=Math.min(sub.effFloor,sub.current);
   const row=node('div',undefined,'bar-row');row.dataset.id=sub.id;
   const bar=node('div',undefined,'bar'),f=node('div',undefined,'bar-floor'),c=node('div',undefined,'bar-current'),p=node('div',undefined,'bar-potential');
   f.style.width=pct(floor);c.style.left=pct(floor);c.style.width=`calc(${pct(sub.current)} - ${pct(floor)})`;p.style.left=pct(sub.current);p.style.width=`calc(${pct(sub.potential)} - ${pct(sub.current)})`;
   for(const [part,el,value] of [['floor',f,sub.effFloor],['current',c,sub.current],['potential',p,sub.potential]]){el.dataset.tip=tips[part];el.dataset.value=value.toFixed(1);}
   f.hidden=floor<=0;c.hidden=sub.current<=floor;p.hidden=sub.potential<=sub.current;
   bar.append(f,c,p);
   const value=node('span',Math.floor(sub.current).toString(),'bar-value');
   const name=node('span',label(sub.id),'bar-name');
   row.append(name,bar,value);list.append(row);
  }
  if(list.childElementCount){details.append(list);out.push(details);}
 }
 $('bars').replaceChildren(...out);
 showTip();
}

// The tooltip follows the mouse; it is re-evaluated after every snapshot (after an action) because
// the bars are rebuilt under a still cursor.
let mouse=null;
function showTip(){
 const tip=$('tip'),el=mouse&&document.elementFromPoint(mouse.x,mouse.y)?.closest('[data-tip]');
 if(!el){tip.hidden=true;return;}
 tip.replaceChildren(node('span',el.dataset.tip),node('small',el.dataset.value));tip.hidden=false;
 const x=Math.min(mouse.x+14,innerWidth-tip.offsetWidth-4),y=mouse.y+18+tip.offsetHeight>innerHeight?mouse.y-tip.offsetHeight-8:mouse.y+18;
 tip.style.left=x+'px';tip.style.top=y+'px';
}
document.addEventListener('mousemove',e=>{mouse={x:e.clientX,y:e.clientY};showTip();});
document.addEventListener('mouseleave',()=>{mouse=null;showTip();});
document.addEventListener('scroll',()=>showTip(),true);

function renderTables(){
 const tables=$('tables');tables.replaceChildren();const next=new Map();
 for(const skill of state.skills||[]){const details=block(skill,`${skill.name} ${skill.F.toFixed(2)} (written ${skill.written})${skill.pending?' · pending':''}`);const line=node('p',`Base ${skill.base.toFixed(2)} · Rust ${skill.R.toFixed(2)} · Idle ${skill.Umain.toFixed(2)} h · Rust/day ${skill.rustPerDay.toFixed(2)} · External ${skill.external}`);line.className='mainline';details.append(line);
 const table=node('table'),head=node('thead'),tr=node('tr');for(const [key,title] of columns){const th=node('th',title);th.tabIndex=0;th.onclick=()=>{sort={key,direction:sort.key===key?-sort.direction:1};render();};th.onkeydown=e=>{if(e.key==='Enter')th.click();};tr.append(th);}head.append(tr);table.append(head);const body=node('tbody');
 for(const sub of [...skill.subs].sort((a,b)=>sort.direction*(typeof a[sort.key]==='string'?a[sort.key].localeCompare(b[sort.key]):a[sort.key]-b[sort.key]))){for(const [key] of columns)next.set(sub.id+'/'+key,sub[key]);if(!visible(sub))continue;
 const row=node('tr');for(const [key] of columns){const value=sub[key],cell=node('td',typeof value==='number'?value.toFixed(key==='worth'?2:1):value);const old=previous.get(sub.id+'/'+key);if(old!==undefined&&old!==value){cell.className='changed';setTimeout(()=>cell.classList.remove('changed'),1000);}row.append(cell);}body.append(row);}
 table.append(body);const scroll=node('div');scroll.className='scroll';scroll.append(table);details.append(scroll);tables.append(details);}
 previous=next;
}

function showTab(tab){for(const id of ['skills','debug'])$(id).hidden=id!==tab;}
document.querySelectorAll('[data-tab]').forEach(b=>b.onclick=()=>showTab(b.dataset.tab));
document.querySelectorAll('[data-action]').forEach(b=>b.onclick=()=>send(b.dataset.action));
$('filter').oninput=render;$('untouched').onchange=render;$('tableView').onchange=render;$('close').onclick=()=>send('close');
$('reinit').onclick=()=>{if($('sure').checked){send('reinitAll');$('sure').checked=false;}};
$('give').onclick=()=>send('giveXP',{id:$('sub').value,raw:Number($('raw').value)});
$('set').onclick=()=>send('setCurrent',{id:$('sub').value,level:Number($('level').value)});
document.addEventListener('keydown',e=>{if(e.key==='Escape'||e.key==='F7'){e.preventDefault();send('close');}});
// PrismaUI loads file:// views and binds spAction after scripts have executed.
// Absence of that binding during startup does not mean browser mock mode.
if(/^https?:$/.test(location.protocol)&&typeof window.spAction!=='function')fetch('mock.json').then(r=>r.json()).then(window.spUpdate).catch(e=>{$('summary').textContent='Could not load mock.json: '+e.message;});
