'use strict';
// Reuse the real panel markup without fetch (file:// must never fetch mock data).
const fixture=document.getElementById('fixture');
for(const id of ['summary','tables','equipped','feedback']){const e=document.createElement('div');e.id=id;fixture.append(e);}
for(const id of ['filter','untouched','sure','raw','level']){const e=document.createElement('input');e.id=id;fixture.append(e);}
for(const id of ['close','reinit','give','set']){const e=document.createElement('button');e.id=id;fixture.append(e);}
const sub=document.createElement('select');sub.id='sub';fixture.append(sub);
const app=document.createElement('script');app.src='app.js';app.onload=()=>{
 const result=document.getElementById('result');
 try{
  if(location.protocol!=='file:')throw Error('Run this regression over file://');
  if(window.fetchCalls!==0)throw Error('Mock fetch attempted before bridge was bound');
  let sent;window.spAction=text=>{sent=JSON.parse(text);};
  window.spUpdate({state:'running',activeHours:0,debug:{xp_mult:1,time_scale:1},skills:[],equipped:[]});
  document.getElementById('close').click();
  if(!sent||sent.action!=='close')throw Error('Late native binding did not work');
  result.dataset.status='pass';result.textContent='PASS: no file mock fetch; late native bridge works';
 }catch(error){result.dataset.status='fail';result.textContent='FAIL: '+error.message;}
};document.body.append(app);
