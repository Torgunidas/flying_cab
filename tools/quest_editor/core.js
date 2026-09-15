/* Flying Cab Story Editor 1.0 — author data and compiler, shared by UI and tests. */
(function (root, factory) {
  const api = factory();
  if (typeof module === 'object' && module.exports) module.exports = api;
  else root.FCStory = api;
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';
  const VERSION = 1;
  const EVENTS = {ride_completed:'Zakończ kurs', passenger_boarded:'Zabierz pasażera', credits_earned:'Zarób kredyty', fuel_purchased:'Kup paliwo', vehicle_repaired:'Napraw pojazd', vehicle_entered:'Wsiądź do pojazdu', vehicle_exited:'Wysiądź', vehicle_taken:'Przejmij pojazd', area_entered:'Wejdź do obszaru', medicine_delivered:'Dostarcz lek', custom:'Zdarzenie z gry'};
  const CONDITIONS = {quest_status:'Stan zadania', fact:'Zapamiętany fakt', credits:'Kredyty', item:'Przedmiot', medicine:'Dawki leku', campaign_active:'Odliczanie trwa', fuel_percent:'Paliwo (%)', vehicle_id:'Prowadzony pojazd', vehicle_model:'Model pojazdu', access:'Uprawnienie'};
  const EFFECTS = {start_quest:'Przyjmij zadanie', turn_in:'Oddaj zadanie i odbierz nagrodę', set_fact:'Zapamiętaj fakt', credit:'Dodaj kredyty', spend:'Pobierz kredyty', grant_item:'Dodaj przedmiot', remove_item:'Zabierz przedmiot', start_campaign:'Rozpocznij odliczanie', buy_medicine:'Kup lek', deliver_medicine:'Podaj lek', grant_access:'Nadaj uprawnienie', revoke_access:'Odbierz uprawnienie'};
  const STATUSES = {inactive:'Przed przyjęciem', active:'W trakcie', ready:'Odbiór nagrody', completed:'Po ukończeniu'};
  const clone = x => JSON.parse(JSON.stringify(x));
  const validId = x => typeof x === 'string' && /^[a-zA-Z0-9][a-zA-Z0-9_.-]*$/.test(x);
  const uid = prefix => prefix + '_' + (globalThis.crypto?.randomUUID?.() || (Date.now().toString(36) + Math.random().toString(36).slice(2))).replace(/-/g, '');
  const isPlayer = n => ['GRACZ','GRACZKA'].includes((n.speaker || '').toUpperCase());
  function marker(lines, line, type) {
    for (let i = line - 1; i >= 0; i--) {
      if (!lines[i].trim()) continue;
      if (!lines[i].trim().startsWith('//')) break;
      const m = lines[i].match(/^\s*\/\/\s*@fc:(node|choice)\s+([\w.-]+)\s*$/);
      if (m) return m[1] === type ? m[2] : null;
    }
    return null;
  }
  function indexed(text, writer) {
    const parsed = writer.parse(text), lines = text.split('\n');
    for (const s of parsed.stages) for (const n of s.nodes) {
      n.stableId = marker(lines, n.sourceLine, 'node');
      for (const o of n.options) o.stableId = marker(lines, o.sourceLine, 'choice');
    }
    return parsed;
  }
  function stabilize(text, writer) {
    const p = indexed(text, writer), lines = text.split('\n'), inserts = [];
    for (const s of p.stages) for (const n of s.nodes) {
      if (!n.stableId) inserts.push({line:n.sourceLine, value:'// @fc:node ' + uid('n')});
      for (const o of n.options) if (!o.stableId) inserts.push({line:o.sourceLine, value:'// @fc:choice ' + uid('c')});
    }
    inserts.sort((a,b) => b.line - a.line).forEach(x => lines.splice(x.line, 0, x.value));
    return lines.join('\n');
  }
  function condition(kind = 'credits', key = '', amount = 1) {
    return {kind, key, amount, comparison:'>=', status:'active', invert:false, reason:'Warunek nie został spełniony.'};
  }
  function effect(kind = 'set_fact', key = '', amount = 1, value = 0) { return {kind, key, amount, value}; }
  function objective() { return {id:uid('cel'), description:'Wykonaj dwa kursy do Depot.', mode:'event', event:'ride_completed', required:2, destination_id:'depot', origin_id:'', target_id:'', vehicle_id:'', custom_event:'', conditions:[], transitions:[]}; }
  function newDocument(npc = 'froggy', name = 'FROGGY', id = uid('quest')) {
    return {id, kind:'quest', title:'Dwa powroty do Depot', description:'Wykonaj dwa kursy do Depot i wróć po zapłatę.', version:1, category:'side', npc, topic_title:'Masz dla mnie pracę?', auto_start:false, requires_turn_in:true, turn_in_npc:npc, reward_credits:75, prerequisites:[], conditions:[], rewards:[], topic_conditions:[], objectives:[objective()], choices:{},
      stages:{ETAP_1:{status:'inactive',conditions:[]},ETAP_2:{status:'active',conditions:[]},ETAP_3:{status:'ready',conditions:[]},ETAP_4:{status:'completed',conditions:[]}},
      text:`##QUEST###${id}\n##NPC\n${name}###${npc}\n\n###ETAP\nETAP_1 ###Oferta ###Przyjmij zadanie.\n##DIALOG *${name}*\n// @fc:node offer\n*${name}*\nZrób dwa kursy do Depot. Zapłacę 75 CR.\n// @fc:node offer_menu\n*GRACZ*\n// @fc:choice accept\nZajmę się tym.{ETAP_2}\n// @fc:choice decline\nNie teraz.{-1}\n\n###ETAP\nETAP_2 ###W trakcie ###Wykonaj kursy w mieście.\n##DIALOG *${name}*\n// @fc:node active\n*${name}*\nWróć po dwóch kursach do Depot.\n// @fc:node active_menu\n*GRACZ*\n// @fc:choice leave_active\nDo zobaczenia.{-1}\n\n###ETAP\nETAP_3 ###Nagroda ###Wróć do zleceniodawcy.\n##DIALOG *${name}*\n// @fc:node ready\n*${name}*\nDobra robota. Oto zapłata.\n// @fc:node ready_menu\n*GRACZ*\n// @fc:choice claim\nOdbieram zapłatę.{ETAP_4}\n\n###ETAP\nETAP_4 ###Zakończone ###Zadanie ukończone.\n##DIALOG *${name}*\n// @fc:node done\n*${name}*\nDzięki za pomoc.\n// @fc:node done_menu\n*GRACZ*\n// @fc:choice leave_done\nNa razie.{-1}\n\n##END\n`};
  }
  function wireTemplate(d) {
    d.choices.accept = {conditions:[], effects:[effect('start_quest',d.id)], once:false, hide_unavailable:false};
    d.choices.claim = {conditions:[], effects:[effect('turn_in',d.id)], once:false, hide_unavailable:false};
    return d;
  }
  function emptyObjective() {
    return {...objective(), description:'', event:'', required:1, destination_id:''};
  }
  function blankQuest(title, npc, id=uid('quest')) {
    const d=wireTemplate(newDocument(npc.id,npc.display_name,id));
    Object.assign(d,{title,description:'',topic_title:title,reward_credits:0,objectives:[],authoring:{from_scratch:true,guide_dismissed:false}});
    const stages=[
      ['ETAP_1','Oferta','offer','oferta zadania','// @fc:choice accept\nZajmę się tym.{ETAP_2}\n// @fc:choice decline\nNie teraz.{-1}'],
      ['ETAP_2','W trakcie','active','przypomnienie celu','// @fc:choice leave_active\nDo zobaczenia.{-1}'],
      ['ETAP_3','Oddanie','ready','odpowiedź po wykonaniu celu','// @fc:choice claim\nZadanie wykonane.{ETAP_4}'],
      ['ETAP_4','Zakończone','done','rozmowa po ukończeniu','// @fc:choice leave_done\nNa razie.{-1}']
    ];
    d.text=`##QUEST###${id}\n##NPC\n${npc.display_name}###${npc.id}\n`+stages.map(([stage,title,node,prompt,choices])=>`\n###ETAP\n${stage} ###${title} ###\n##DIALOG *${npc.display_name}*\n// @fc:node ${node}\n*${npc.display_name}*\n⟪Uzupełnij: ${prompt}⟫\n// @fc:node ${node}_menu\n*GRACZ*\n${choices}\n`).join('')+'\n##END\n';
    return d;
  }
  function createQuest(project, input, world={}) {
    checkProject(project);
    if(!['current','new'].includes(input.scope))throw new Error('Wybierz, gdzie zapisać quest.');
    const title=String(input.title||'').trim();
    if(!title)throw new Error('Wpisz nazwę nowego questa.');
    const next=input.scope==='current'?clone(project):{format:'flying-cab-story',version:VERSION,id:uid('fabula'),title,documents:[],npcs:clone(world.npcs||project.npcs.filter(n=>n.existing)),items:[],fact_ids:[],access_ids:[],custom_events:[]};
    let npc=next.npcs.find(n=>n.id===input.npc_id);
    if(input.npc_id==='__new__'){
      const name=String(input.new_npc_name||'').trim().toUpperCase();
      if(!name||/[\r\n*{}#⟪⟫]/.test(name)||['GRACZ','GRACZKA'].includes(name))throw new Error('Wpisz imię postaci bez znaczników składni dialogu.');
      if(next.npcs.some(n=>n.display_name.toUpperCase()===name))throw new Error('Postać o tym imieniu już istnieje. Wybierz ją z listy.');
      npc={id:uid('npc'),display_name:name,greeting:'O czym chcesz porozmawiać?',existing:false,portrait:''};
      next.npcs.push(npc);
    }
    if(!npc)throw new Error('Wybierz zleceniodawcę zadania.');
    next.documents.push(blankQuest(title,npc));
    return next;
  }
  function newConversation(npc, name) {
    const d = newDocument(npc, name, uid('rozmowa'));
    Object.assign(d, {kind:'dialogue', title:'Nowa rozmowa', topic_title:'Porozmawiajmy.', objectives:[], choices:{}, stages:{ETAP_1:{status:'always',conditions:[]}}});
    d.text = `##QUEST###${d.id}\n##NPC\n${name}###${npc}\n\n###ETAP\nETAP_1 ###Rozmowa ###\n##DIALOG *${name}*\n// @fc:node greeting\n*${name}*\nDobrze cię widzieć.\n// @fc:node menu\n*GRACZ*\n// @fc:choice goodbye\nDo zobaczenia.{-1}\n\n##END\n`;
    return d;
  }
  function renameSpeaker(text, before, after) {
    return text.split('\n').map(line => line.trim() === '*'+before+'*' ? '*'+after+'*' : line.trim() === '##DIALOG *'+before+'*' ? '##DIALOG *'+after+'*' : line.startsWith(before+'###') ? after+line.slice(before.length) : line).join('\n');
  }
  // Keep author-facing renames attached to their gameplay references.
  function updateField(project, path, value) {
    const keys=path.split('.'), key=keys.at(-1), owner=keys.slice(0,-1).reduce((v,k)=>v[k],project), old=owner[key];
    if(old===value)return;
    if(key==='id'&&!validId(value))throw new Error('ID może zawierać litery, cyfry, podkreślenie, kropkę i myślnik.');
    const group=keys[0], isDoc=group==='documents'&&keys.length===3, isNpc=group==='npcs'&&keys.length===3;
    function walk(value, fn) { if(!value||typeof value!=='object')return;fn(value);for(const v of Object.values(value))walk(v,fn); }
    function references(kinds){walk(project,x=>{if(kinds.includes(x.kind)&&x.key===old)x.key=value;});}
    if(isDoc&&key==='id'){
      references(['quest_status','start_quest','turn_in']);
      for(const d of project.documents)d.prerequisites=d.prerequisites.map(x=>x===old?value:x);
      owner.text=owner.text.replace('##QUEST###'+old,'##QUEST###'+value);
    }else if(isDoc&&key==='npc'){
      const a=project.npcs.find(n=>n.id===old), b=project.npcs.find(n=>n.id===value);
      if(a&&b)owner.text=renameSpeaker(owner.text,a.display_name,b.display_name).replace(a.display_name+'###'+a.id,b.display_name+'###'+b.id).replace(b.display_name+'###'+a.id,b.display_name+'###'+b.id);
      if(owner.turn_in_npc===old)owner.turn_in_npc=value;
    }else if(isNpc&&key==='display_name'){
      for(const d of project.documents)d.text=renameSpeaker(d.text,old,value);
    }else if(isNpc&&key==='id'){
      for(const d of project.documents){if(d.npc===old)d.npc=value;if(d.turn_in_npc===old)d.turn_in_npc=value;d.text=d.text.replace(owner.display_name+'###'+old,owner.display_name+'###'+value);for(const o of d.objectives)if(o.event==='medicine_delivered'&&o.target_id===old)o.target_id=value;}
    }else if(group==='documents'&&keys[2]==='objectives'&&keys.length===5&&key==='event'){
      if(!['ride_completed','passenger_boarded'].includes(old)||!['ride_completed','passenger_boarded'].includes(value)){owner.origin_id='';owner.destination_id='';}
      owner.target_id='';owner.custom_event='';
      if(['credits_earned','medicine_delivered'].includes(value))owner.vehicle_id='';
      if(!['credits_earned','fuel_purchased','vehicle_repaired'].includes(value)&&!Number.isInteger(owner.required))owner.required=1;
    }else if(group==='documents'&&keys[2]==='objectives'&&keys.length===5&&key==='id'){
      for(const o of project.documents[Number(keys[1])].objectives)for(const t of o.transitions||[])if(t.target===old)t.target=value;
    }else if(group==='fact_ids'&&keys.length===2)references(['fact','set_fact']);
    else if(group==='access_ids'&&keys.length===2)references(['access','grant_access','revoke_access']);
    else if(group==='custom_events'&&keys.length===2){for(const d of project.documents)for(const o of d.objectives)if(o.custom_event===old)o.custom_event=value;}
    else if(group==='items'&&key==='id')references(['item','grant_item','remove_item']);
    owner[key]=value;
  }
  function example(world = {}) {
    return {format:'flying-cab-story', version:VERSION, id:'cab_story', title:'Flying Cab — fabuła', documents:[wireTemplate(newDocument('froggy','FROGGY','fc_depot_runs'))], npcs:clone(world.npcs || [{id:'froggy',display_name:'FROGGY',existing:true,greeting:'Czego potrzebujesz, kierowco?'},{id:'maya',display_name:'MAYA',existing:true,greeting:'Dobrze cię widzieć.'}]), items:[], fact_ids:[], access_ids:[], custom_events:[]};
  }
  function checkProject(p) {
    if (!p || p.format !== 'flying-cab-story' || p.version !== VERSION || !validId(p.id) || typeof p.title !== 'string' || !Array.isArray(p.documents) || !Array.isArray(p.npcs)) throw new Error('To nie jest projekt Flying Cab Story w wersji 1.');
    for (const key of ['items','fact_ids','access_ids','custom_events']) if (!Array.isArray(p[key])) throw new Error('Brak listy: ' + key);
    if(!p.documents.length)throw new Error('Projekt musi zawierać przynajmniej jeden dokument.');
    for(const npc of p.npcs)if(!npc||!validId(npc.id)||typeof npc.display_name!=='string'||typeof npc.greeting!=='string')throw new Error('Nieprawidłowy profil postaci.');
    for (const d of p.documents) {
      if (!d || !validId(d.id) || typeof d.text !== 'string' || !d.stages || !d.choices || !Array.isArray(d.objectives)) throw new Error('Nieprawidłowy dokument projektu.');
      for(const key of ['conditions','rewards','prerequisites','topic_conditions'])if(!Array.isArray(d[key]))throw new Error('Dokument '+d.id+': brak listy '+key);
      for(const o of d.objectives)if(!o||!Array.isArray(o.conditions)||!Array.isArray(o.transitions)||o.transitions.some(t=>!t||!Array.isArray(t.conditions)))throw new Error('Dokument '+d.id+': nieprawidłowy cel lub przejście.');
      for(const s of Object.values(d.stages))if(!s||!Array.isArray(s.conditions))throw new Error('Dokument '+d.id+': nieprawidłowy początek rozmowy.');
      for(const c of Object.values(d.choices))if(!c||!Array.isArray(c.conditions)||!Array.isArray(c.effects))throw new Error('Dokument '+d.id+': nieprawidłowe ustawienia odpowiedzi.');
    }
    return p;
  }
  function compile(project, writer, world = {}) {
    checkProject(project);
    const errors = [], warnings = [], issue = (message, doc, line) => errors.push({message, doc, line});
    const out = {format:'flying-cab-narrative', version:1, project_id:project.id, quests:[], dialogues:[], npcs:clone(project.npcs), items:clone(project.items), fact_ids:clone(project.fact_ids), access_ids:clone(project.access_ids), custom_events:clone(project.custom_events)};
    const known = {
      quest:new Set([...(world.quests || []).map(x=>x.id), ...project.documents.filter(d=>d.kind==='quest').map(d=>d.id)]),
      npc:new Set(project.npcs.map(n=>n.id)), item:new Set([...(world.items || []).map(x=>x.id),...project.items.map(x=>x.id)]),
      fact:new Set([...(world.fact_ids || []),...project.fact_ids]), access:new Set([...(world.access_ids || []),...project.access_ids]),
      custom:new Set([...(world.custom_events || []),...project.custom_events]), stop:new Set((world.stops || []).map(x=>x.id))
    };
    for (const [label, ids] of [['dokument',project.documents.map(x=>x.id)],['NPC',project.npcs.map(x=>x.id)],['przedmiot',project.items.map(x=>x.id)],['fakt',project.fact_ids],['uprawnienie',project.access_ids],['zdarzenie',project.custom_events]]) {
      const seen = new Set();
      for (const id of ids) { if (!validId(id) || seen.has(id)) issue('Nieprawidłowe lub powtórzone ID: '+label+' '+id); seen.add(id); }
    }
    const requireKey = (kind,key,at) => { if (known[kind] && !known[kind].has(key)) issue(at+': nieznany '+kind+' „'+key+'”.'); };
    function conditions(list,at) {
      if (!Array.isArray(list)) { issue(at+': brak listy warunków.'); return []; }
      return list.map(c=>{
        if (!c || !Object.hasOwn(CONDITIONS,c.kind) || !Number.isFinite(c.amount) || !['>=','<=','==','!=','>','<'].includes(c.comparison) || typeof c.invert !== 'boolean') issue(at+': nieprawidłowy warunek.');
        else { requireKey(c.kind==='quest_status'?'quest':c.kind,c.key,at); if (c.kind==='quest_status'&&!Object.hasOwn(STATUSES,c.status)) issue(at+': nieprawidłowy stan zadania.'); }
        return clone(c);
      });
    }
    function effects(list,at,reward=false) {
      if (!Array.isArray(list)) { issue(at+': brak listy skutków.'); return []; }
      return list.map(e=>{
        if (!e || !Object.hasOwn(EFFECTS,e.kind) || !Number.isFinite(e.amount) || e.amount<0 || !Number.isFinite(e.value) || e.value<0) issue(at+': nieprawidłowy skutek.');
        else {
          requireKey(({start_quest:'quest',turn_in:'quest',set_fact:'fact',grant_item:'item',remove_item:'item',grant_access:'access',revoke_access:'access'})[e.kind],e.key,at);
          if (['grant_item','remove_item','buy_medicine','deliver_medicine'].includes(e.kind) && (!Number.isSafeInteger(e.amount)||e.amount<1)) issue(at+': liczba przedmiotów/dawek musi być całkowita i dodatnia.');
          if (['buy_medicine','deliver_medicine'].includes(e.kind)&&e.amount>1000) issue(at+': najwyżej 1000 dawek.');
          if ((e.kind==='deliver_medicine'&&e.value<=0)||(e.kind==='start_campaign'&&e.amount<=0)) issue(at+': czas musi być dodatni.');
          if (reward&&!['set_fact','grant_item','grant_access','revoke_access'].includes(e.kind)) issue(at+': ten skutek nie jest nagrodą zadania.');
        }
        return clone(e);
      });
    }
    for (const npc of out.npcs) { npc.topics=[]; if (!npc.display_name?.trim()) issue('NPC '+npc.id+': wpisz imię.'); }
    for (const doc of project.documents) {
      const at = doc.title || doc.id;
      if (!['quest','dialogue'].includes(doc.kind)) issue(at+': nieznany typ dokumentu.',doc.id);
      if (!doc.title?.trim() || !doc.topic_title?.trim()) issue(at+': wpisz tytuł i temat rozmowy.',doc.id);
      requireKey('npc',doc.npc,at);
      const parsed = indexed(doc.text,writer);
      doc.text.split('\n').forEach((line,i)=>{if(line.includes('⟪Uzupełnij:'))issue(at+': zastąp wskazówkę własnym tekstem — '+line.trim(),doc.id,i);});
      for (const e of writer.lint(parsed)) issue(at+': '+e.message,doc.id,e.gotoLine);
      if (!parsed.stages.length) { issue(at+': dodaj etap i rozmowę.',doc.id); continue; }
      const stageIds = new Set();
      for (const line of doc.text.split('\n')) { const m=line.match(/^ETAP_(\w+)\s*###/); if(m){if(stageIds.has(m[1]))issue(at+': powtórzony nagłówek etapu.',doc.id);stageIds.add(m[1]);} }
      const nodes = parsed.stages.flatMap(s=>s.nodes.map(n=>({n,s}))), byParser = new Map(nodes.map(x=>[x.n.id,x])), seen = new Set();
      for (const {n} of nodes) {
        for (const x of [n,...n.options]) { if(!x.stableId||!/^[a-zA-Z0-9][a-zA-Z0-9_-]*$/.test(x.stableId)||seen.has(x.stableId)) issue(at+': brak, powtórzone lub niepoprawne ID wypowiedzi (litery, cyfry, _ i -). Użyj „Uzupełnij ID” dla nowych wpisów.',doc.id,x.sourceLine); seen.add(x.stableId); }
        if (!isPlayer(n) && !project.npcs.some(p=>p.display_name.toUpperCase()===n.speaker.toUpperCase())) issue(at+': postać „'+n.speaker+'” nie jest w katalogu.',doc.id,n.sourceLine);
        if (/\[(?:\/?[mfo])\]/i.test(n.body)) issue(at+': tagi wariantów [m]/[f]/[o] wymagają zastąpienia jawnymi odpowiedziami.',doc.id,n.sourceLine);
      }
      function nextInDialog(s,n) { const next=s.nodes[s.nodes.indexOf(n)+1]; return next&&next.dialogIndex===n.dialogIndex ? next : null; }
      function first(s) { const n=s?.nodes[0]; return n?.stableId || ''; }
      function resolve(s,n,target) {
        if (target==='-1') return '';
        if (!target) return nextInDialog(s,n)?.stableId || '';
        if (target.startsWith('ETAP_')) return first(parsed.stages.find(x=>x.id===target));
        const id=s.dialogAnchors[n.dialogIndex]?.[target];
        return byParser.get(id)?.n.stableId || '';
      }
      const dialogue={id:doc.id+'_dialogue',start_node:first(parsed.stages[0]),nodes:[],entries:[]};
      for (const s of parsed.stages) {
        const cfg=doc.stages[s.id] || {status:'none',conditions:[]};
        if (s.conditions.length) issue(at+' / '+s.id+': //warunek pozostaje opisem. Ustaw wykonanie celu w panelu Zadanie, a ten komentarz zmień na zwykłą notatkę.',doc.id,s.sourceLine);
        if (!s.nodes.length) { issue(at+' / '+s.id+': etap rozmowy jest pusty.',doc.id,s.sourceLine); continue; }
        if (isPlayer(s.nodes[0])) issue(at+' / '+s.id+': zacznij rozmowę wypowiedzią NPC.',doc.id,s.sourceLine);
        const entryConditions=conditions(cfg.conditions||[],at+' / '+s.id);
        if (Object.hasOwn(STATUSES,cfg.status)) {
          if(doc.kind!=='quest')issue(at+': stany własnego questa są dostępne w dokumencie Zadanie.',doc.id);
          entryConditions.unshift({...condition('quest_status',doc.id),status:cfg.status});
        }
        if (cfg.status!=='none') dialogue.entries.push({node_id:first(s),conditions:entryConditions});
        for (const n of s.nodes) {
          const prev=s.nodes[s.nodes.indexOf(n)-1], next=nextInDialog(s,n);
          if (isPlayer(n) && !n.anchor) continue; // ordinary menus belong to the preceding NPC line
          let menu=isPlayer(n)?n:(next&&isPlayer(next)?next:null);
          const body=isPlayer(n)?(prev&&!isPlayer(prev)?prev.body:'Wybierz odpowiedź.'):n.body;
          const runtime={id:n.stableId,speaker:isPlayer(n)?(prev?.speaker||''):n.speaker,text:body.replace(/\[\/?sb\]/g,'').trim(),choices:[]};
          if (!runtime.text) issue(at+': pusta wypowiedź NPC.',doc.id,n.sourceLine);
          if (menu && !n.outgoing.length) {
            for (const o of menu.options) {
              const settings=doc.choices[o.stableId]||{conditions:[],effects:[],once:false,hide_unavailable:false};
              let target=resolve(s,menu,o.target);
              const targetNode=nodes.find(x=>x.n.stableId===target)?.n;
              if(targetNode&&isPlayer(targetNode)&&!targetNode.anchor) issue(at+': skok do menu wymaga kotwicy.',doc.id,o.sourceLine);
              runtime.choices.push({id:o.stableId,text:o.text,next_node:target,conditions:conditions(settings.conditions,at+' / '+o.text),effects:effects(settings.effects,at+' / '+o.text),once:!!settings.once,hide_unavailable:!!settings.hide_unavailable});
              if(o.text.length>40)warnings.push({message:at+': sprawdź długość odpowiedzi „'+o.text+'” na ekranie telefonu.',doc:doc.id,line:o.sourceLine});
            }
          } else {
            const target=n.outgoing.length?resolve(s,n,n.outgoing[0]):(next?.stableId||'');
            runtime.choices.push({id:'continue',text:target?'Dalej.':'Do zobaczenia.',next_node:target,conditions:[],effects:[],once:false,hide_unavailable:false});
          }
          dialogue.nodes.push(runtime);
        }
      }
      for (const id of Object.keys(doc.stages)) if(!parsed.stageMap[id])issue(at+': ustawienia wskazują usunięty etap '+id+'.',doc.id);
      for(const n of dialogue.nodes)for(const c of n.choices)if(c.next_node&&!dialogue.nodes.some(x=>x.id===c.next_node))issue(at+': przejście nie wskazuje wypowiedzi: '+c.next_node,doc.id);
      out.dialogues.push(dialogue);
      const npc=out.npcs.find(n=>n.id===doc.npc);
      if(npc)npc.topics.push({id:doc.id, title:doc.topic_title, dialogue:dialogue.id, conditions:conditions(doc.topic_conditions||[],at+' / temat'),hide_unavailable:false});
      if(doc.kind==='quest') {
        if(!Number.isInteger(doc.version)||doc.version<1||!Number.isFinite(doc.reward_credits)||doc.reward_credits<0)issue(at+': nieprawidłowa wersja lub nagroda.',doc.id);
        if(!doc.objectives.length)issue(at+': dodaj przynajmniej jeden cel.',doc.id);
        if(doc.requires_turn_in)requireKey('npc',doc.turn_in_npc,at);
        for(const id of doc.prerequisites||[])requireKey('quest',id,at);
        const q={id:doc.id,title:doc.title,description:doc.description,version:doc.version,category:doc.category,auto_start:!!doc.auto_start,requires_turn_in:!!doc.requires_turn_in,turn_in_npc:doc.turn_in_npc,reward_credits:doc.reward_credits,prerequisites:clone(doc.prerequisites||[]),conditions:conditions(doc.conditions,at),rewards:effects(doc.rewards,at,true),objectives:[]};
        const ids=new Set();
        for(const o of doc.objectives) {
          if(!validId(o.id)||ids.has(o.id)||!Number.isFinite(o.required)||o.required<=0||!['event','state'].includes(o.mode))issue(at+': nieprawidłowy cel '+o.id,doc.id);
          if(!o.description?.trim())issue(at+': wpisz opis celu '+(q.objectives.length+1)+' w panelu Zadanie.',doc.id);
          if(!Object.hasOwn(EVENTS,o.event)&&!(o.mode==='state'&&o.event===''))issue(at+': wybierz zdarzenie zaliczające cel '+(q.objectives.length+1)+'.',doc.id);
          ids.add(o.id);
          if(o.mode==='event'&&!['credits_earned','fuel_purchased','vehicle_repaired'].includes(o.event)&&!Number.isInteger(o.required))issue(at+': liczba zdarzeń musi być całkowita.',doc.id);
          if(o.mode==='state'&&!o.conditions.length)issue(at+': cel sprawdzający stan potrzebuje warunku.',doc.id);
          if(o.event==='custom')requireKey('custom',o.custom_event,at);
          if(o.mode==='event'&&!['ride_completed','passenger_boarded','custom'].includes(o.event)&&(o.origin_id||o.destination_id))issue(at+': to zdarzenie nie wysyła platformy początku/końca. Wyczyść filtry lub ponownie wybierz rodzaj zdarzenia.',doc.id);
          if(o.mode==='event'&&['credits_earned','medicine_delivered'].includes(o.event)&&o.vehicle_id)issue(at+': to zdarzenie nie wskazuje pojazdu. Wyczyść filtr pojazdu.',doc.id);
          for(const k of ['origin_id','destination_id'])if(o[k]&&known.stop.size&&!known.stop.has(o[k]))issue(at+': nieznana platforma '+o[k],doc.id);
          q.objectives.push({...clone(o),event:o.event||(o.mode==='state'?'ride_completed':''),conditions:conditions(o.conditions,at+' / cel'),transitions:(o.transitions||[]).map(t=>({target:t.target,conditions:conditions(t.conditions,at+' / przejście')}))});
        }
        for(const o of q.objectives) {
          if(o.transitions.length&&o.transitions.at(-1).conditions.length)issue(at+': ostatnie przejście celu musi być „w pozostałych przypadkach”.',doc.id);
          for(const [i,t] of o.transitions.entries()) {
            if(t.target&&!ids.has(t.target))issue(at+': przejście do nieznanego celu '+t.target,doc.id);
            if(!t.conditions.length&&i<o.transitions.length-1)issue(at+': bezwarunkowe przejście zasłania dalsze reguły.',doc.id);
          }
        }
        const visited=new Set(),stack=new Set();
        function visit(id){if(stack.has(id)){issue(at+': cele tworzą pętlę. Użyj osobnego zadania dla powtórzenia.',doc.id);return;}if(visited.has(id))return;visited.add(id);stack.add(id);const i=q.objectives.findIndex(o=>o.id===id),o=q.objectives[i];if(o){const targets=o.transitions.length?o.transitions.map(t=>t.target):[q.objectives[i+1]?.id];for(const target of targets)if(target)visit(target);}stack.delete(id);}
        if(q.objectives[0])visit(q.objectives[0].id);
        for(const o of q.objectives)if(!visited.has(o.id))issue(at+': nieosiągalny cel '+o.description,doc.id);
        out.quests.push(q);
      }
    }
    for(const q of out.quests) {
      const actions=out.dialogues.flatMap(d=>d.nodes.flatMap(n=>n.choices.flatMap(c=>c.effects)));
      if(!q.auto_start&&!actions.some(e=>e.kind==='start_quest'&&e.key===q.id))issue(q.title+': żadna odpowiedź nie przyjmuje zadania.',q.id);
      if(q.requires_turn_in&&!actions.some(e=>e.kind==='turn_in'&&e.key===q.id))issue(q.title+': żadna odpowiedź nie odbiera nagrody.',q.id);
      else if(q.requires_turn_in){const npc=out.npcs.find(n=>n.id===q.turn_in_npc);const canClaim=npc?.topics.some(t=>out.dialogues.find(d=>d.id===t.dialogue)?.nodes.some(n=>n.choices.some(c=>c.effects.some(e=>e.kind==='turn_in'&&e.key===q.id))));if(!canClaim)issue(q.title+': odpowiedź odbierająca nagrodę musi należeć do rozmowy z wyznaczonym odbiorcą.',q.id);}
    }
    return {bundle:out,errors,warnings};
  }
  return {VERSION,EVENTS,CONDITIONS,EFFECTS,STATUSES,clone,validId,uid,isPlayer,indexed,stabilize,condition,effect,objective,emptyObjective,blankQuest,createQuest,newDocument,newConversation,updateField,wireTemplate,example,checkProject,compile};
});
