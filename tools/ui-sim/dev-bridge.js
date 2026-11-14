'use strict';
// UI simulator bridge (tools/ui-sim/README.md). Injected by serve.py before
// app.js; never shipped with the mod. Replaces the native PrismaUI binding
// window.spAction with HTTP calls to PracticeSim and adds a "Simulator" panel:
// simulated player actions, time, settings, start level and an aim view.
(() => {
  let last = null, presets = [], autoTimer = null, aimKey = '';
  const messages = [];

  async function post(action) {
    let snap;
    try {
      const r = await fetch('/sim/action', {method: 'POST', body: JSON.stringify(action)});
      snap = await r.json();
      if (!r.ok) throw Error(snap.error || r.statusText);
    } catch (e) {
      log('error: ' + e.message, true);
      return;
    }
    last = snap;
    for (const m of snap.sim.messages) log(m);
    if (snap.sim.error) log(action.action + ': ' + snap.sim.error, true);
    if (typeof window.spUpdate === 'function') window.spUpdate(snap);
    render();
  }
  // The real panel calls this exactly as in the game.
  window.spAction = text => post(JSON.parse(text));

  const h = (tag, props = {}, ...kids) => {
    const n = Object.assign(document.createElement(tag), props);
    n.append(...kids.flat());
    return n;
  };
  const $ = id => document.getElementById(id);

  function log(text, error = false) {
    messages.unshift({text, error, time: new Date().toLocaleTimeString()});
    messages.length = Math.min(messages.length, 12);
    const box = $('sim-log');
    if (box) box.replaceChildren(...messages.map(m =>
      h('div', {className: m.error ? 'sim-error' : '', textContent: `${m.time}  ${m.text}`})));
  }

  function setting(section, key, value) { post({action: 'set', section, key, value}); }

  function act(p) {
    const times = Math.max(1, Number($('sim-times').value) || 1);
    const override = Number($('sim-points').value);
    const points = override > 0 ? override : p.points;
    const send = () => post({action: 'use', skill: p.skill, subs: p.subs, points, times});
    if (p.equip) {
      const right = Object.assign({hand: 'right'}, p.equip);
      const left = (last?.equipped || []).find(e => e.hand === 'left') ||
                   {hand: 'left', kind: 'none', name: 'Unarmed'};
      post({action: 'equip', equipped: [right, left]}).then(send);
    } else send();
  }

  function pass(hours) { post({action: 'tick', hours}); }

  function fillPresets() {
    const groups = new Map();
    for (const p of presets) {
      if (!groups.has(p.group)) groups.set(p.group, []);
      groups.get(p.group).push(h('button', {textContent: p.label, title: `${p.points} pts → ${p.subs.map(s => Array.isArray(s) ? `${s[0]}×${s[1]}` : s).join(', ')}`, onclick: () => act(p)}));
    }
    $('sim-actions').replaceChildren(...[...groups].map(([g, buttons]) =>
      h('div', {className: 'sim-group'}, h('span', {textContent: g}), h('div', {className: 'sim-row'}, buttons))));
  }

  function buildPanel() {
    document.body.classList.add('sim');
    const select = (id, values, key) => h('select', {id, onchange: e => setting(...key, Number(e.target.value))},
      values.map(v => h('option', {value: v, textContent: '×' + v})));
    const check = (id, label, key) => h('label', {}, h('input', {type: 'checkbox', id, onchange: e => setting(...key, e.target.checked)}), label);
    const panel = h('aside', {id: 'sim-panel'},
      h('h2', {textContent: 'Simulator'}),
      h('p', {className: 'sim-note', textContent: 'Real model (PracticeCore), no game: actions name the sub-skills directly; points are placeholders (R-17).'}),
      h('section', {},
        h('h3', {textContent: 'Player actions'}),
        h('div', {className: 'sim-row'},
          h('label', {}, 'Repeat', h('input', {id: 'sim-times', type: 'number', min: 1, value: 1})),
          h('label', {}, 'Points', h('input', {id: 'sim-points', type: 'number', min: 0, placeholder: 'preset'}))),
        h('div', {id: 'sim-actions', textContent: 'loading presets…'})),
      h('section', {},
        h('h3', {textContent: 'Active game time'}),
        h('div', {className: 'sim-row'},
          [1, 8, 24, 72, 240].map(n => h('button', {textContent: `+${n} h`, onclick: () => pass(n)})),
          h('input', {id: 'sim-hours', type: 'number', min: 0, value: 2}),
          h('button', {textContent: 'Pass', onclick: () => pass(Number($('sim-hours').value) || 0)})),
        h('div', {className: 'sim-row'},
          h('label', {}, h('input', {type: 'checkbox', id: 'sim-auto', onchange: auto}), 'Run'),
          h('input', {id: 'sim-rate', type: 'number', min: 0, step: 0.05, value: 0.1, onchange: auto}),
          h('span', {textContent: 'game h / real s (decay also × time scale)'}))),
      h('section', {},
        h('h3', {textContent: 'Settings (MCM in the game)'}),
        h('div', {className: 'sim-row'},
          h('label', {}, 'XP', select('sim-xp', [1, 10, 100], ['debug', 'xp_mult'])),
          h('label', {}, 'Time', select('sim-time', [1, 10, 100, 1000], ['debug', 'time_scale']))),
        h('div', {className: 'sim-row'},
          check('sim-grace', 'No grace', ['debug', 'no_grace']),
          check('sim-write', 'Write main', ['main', 'write_main_skills']),
          check('sim-missile', 'Missile noise', ['aim', 'missile_enabled']),
          check('sim-beam', 'Beam sway', ['aim', 'beam_enabled'])),
        h('div', {className: 'sim-row'},
          h('label', {}, 'Start level', h('input', {id: 'sim-base', type: 'number', min: 0, max: 100, value: 15})),
          h('button', {textContent: 'Re-initialize', onclick: () => post({action: 'reinitAll', base: Number($('sim-base').value)})}),
          h('button', {textContent: 'Reload TOML', onclick: () => post({action: 'reloadConfig'})}))),
      h('section', {},
        h('h3', {textContent: 'Aim (right hand)'}),
        h('div', {className: 'sim-row'},
          h('label', {}, 'Distance, m', h('input', {id: 'sim-distance', type: 'number', min: 1, value: 20, onchange: () => { aimKey = ''; render(); }})),
          h('button', {textContent: 'Resample', onclick: () => { aimKey = ''; render(); }})),
        h('canvas', {id: 'sim-aim', width: 300, height: 300}),
        h('p', {id: 'sim-aim-text', className: 'sim-note'})),
      h('section', {}, h('h3', {textContent: 'Log'}), h('div', {id: 'sim-log'})));
    document.body.append(panel);
  }

  function auto() {
    clearInterval(autoTimer);
    autoTimer = null;
    if ($('sim-auto').checked) {
      const rate = Number($('sim-rate').value) || 0;
      autoTimer = setInterval(() => pass(rate / 4), 250);
    }
  }

  function render() {
    if (!last || !$('sim-panel')) return;
    const d = last.sim.debugSettings, a = last.sim.aim;
    $('sim-xp').value = d.xp_mult;
    $('sim-time').value = d.time_scale;
    $('sim-grace').checked = !!d.no_grace;
    $('sim-write').checked = !!last.debug.write_main_skills;
    $('sim-missile').checked = !!a.missile_enabled;
    $('sim-beam').checked = !!a.beam_enabled;
    if (document.activeElement !== $('sim-base')) $('sim-base').value = last.sim.base;
    drawAim(a);
  }

  // Gaussian sample (Box-Muller), degrees in, degrees out.
  const gauss = s => s * Math.sqrt(-2 * Math.log(1 - Math.random())) * Math.cos(2 * Math.PI * Math.random());

  function drawAim(a) {
    const right = (last.equipped || []).find(e => e.hand === 'right' && e.kind === 'spell');
    const dist = Number($('sim-distance').value) || 20;
    const key = JSON.stringify([right?.name, right?.sigma, right?.sway, dist, a]);
    if (key === aimKey) return;
    aimKey = key;
    const c = $('sim-aim'), g = c.getContext('2d'), text = $('sim-aim-text');
    const W = c.width, half = 3;  // the canvas shows ±3 m around the aim point
    const px = m => W / 2 + m * W / (2 * half);
    g.fillStyle = '#11151a'; g.fillRect(0, 0, W, W);
    g.strokeStyle = '#3a424b';
    for (let m = -half; m <= half; ++m) {
      g.beginPath(); g.moveTo(px(m), 0); g.lineTo(px(m), W); g.stroke();
      g.beginPath(); g.moveTo(0, px(m)); g.lineTo(W, px(m)); g.stroke();
    }
    // A standing human: 0.5 m wide, 1.8 m tall, aimed at the chest.
    g.fillStyle = '#4a5560';
    g.fillRect(px(-0.25), px(-0.5), px(0.25) - px(-0.25), px(1.3) - px(-0.5));
    const toM = deg => dist * Math.tan(deg * Math.PI / 180);
    if (!right || (right.form !== 'missile' && right.form !== 'beam')) {
      text.textContent = 'Use a missile or beam spell preset (Ice Spike, Lightning Bolt, Sun Fire) to equip it.';
      return;
    }
    if (right.form === 'missile') {
      let hits = 0;
      g.fillStyle = '#f6d778';
      for (let i = 0; i < 300; ++i) {
        const x = toM(gauss(right.sigma)), y = toM(gauss(right.sigma));
        if (Math.abs(x) <= 0.25 && y >= -0.5 && y <= 1.3) ++hits;
        g.fillRect(px(x) - 1.5, px(y) - 1.5, 3, 3);
      }
      text.textContent = `${right.name}: form level ${right.formLevel.toFixed(1)}, σ = ${right.sigma.toFixed(2)}° → ${(hits / 3).toFixed(0)} % of 300 shots hit a person at ${dist} m. Model of the formula only; the real launch is a game check (R-26).`;
    } else {
      // Same curve as runtime::refreshAim (Aim.cpp), sampled over 6 s of holding.
      const amp = right.sway, w = a.beam_secondary_weight, f = a.beam_noise_hz;
      const p1 = Math.random() * 2 * Math.PI, p2 = Math.random() * 2 * Math.PI;
      g.strokeStyle = '#f6d778'; g.beginPath();
      for (let t = 0; t <= 6; t += 0.02) {
        const k = Math.min(1, t / a.beam_t_full_sec), s = t * 2 * Math.PI * f;
        const x = amp * k * (Math.sin(s * a.beam_yaw_primary_frequency + p2) + w * Math.sin(s * a.beam_yaw_secondary_frequency + p1)) / (1 + w);
        const y = amp * k * (Math.sin(s + p1) + w * Math.sin(s * a.beam_pitch_secondary_frequency + p2)) / (1 + w);
        t === 0 ? g.moveTo(px(toM(x)), px(toM(y))) : g.lineTo(px(toM(x)), px(toM(y)));
      }
      g.stroke();
      text.textContent = `${right.name}: form level ${right.formLevel.toFixed(1)}, full sway amplitude ${amp.toFixed(2)}° (after ${a.beam_t_full_sec} s), path of 6 s at ${dist} m. Feel and camera behaviour are game checks.`;
    }
  }

  document.addEventListener('DOMContentLoaded', () => {
    buildPanel();
    post({action: 'snapshot'});
    fetch('/sim/presets.json').then(r => r.json())
      .then(j => { presets = j.actions; fillPresets(); })
      .catch(e => log('presets.json: ' + e.message, true));
  });
})();
