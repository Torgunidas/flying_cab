'use strict';
const fs=require('node:fs'), path=require('node:path'), vm=require('node:vm');
function makeStubElement() {
  const el = {};
  const listHandler = {
    add() {}, remove() {}, toggle() {}, contains() { return false; }
  };
  return new Proxy(el, {
    get(target, prop) {
      if (prop === 'style') return target.__style || (target.__style = new Proxy({}, { get: () => '', set: () => true }));
      if (prop === 'classList') return listHandler;
      if (prop === 'dataset') return target.__dataset || (target.__dataset = {});
      if (prop === 'children' || prop === 'childNodes' || prop === 'options' ||
          prop === 'selectedOptions' || prop === 'rows' || prop === 'cells' ||
          prop === 'files' || prop === 'attributes') return [];
      if (prop === 'value' || prop === 'textContent' || prop === 'innerHTML' || prop === 'outerHTML') {
        return target['__' + String(prop)] !== undefined ? target['__' + String(prop)] : '';
      }
      if (prop === 'selectionStart' || prop === 'selectionEnd' || prop === 'scrollTop' ||
          prop === 'scrollLeft' || prop === 'scrollHeight' || prop === 'clientHeight' ||
          prop === 'clientWidth' || prop === 'offsetTop' || prop === 'offsetHeight' ||
          prop === 'offsetWidth') return 0;
      if (prop === 'getBoundingClientRect') return () => ({ top: 0, left: 0, right: 0, bottom: 0, width: 0, height: 0 });
      if (prop === 'querySelector') return () => null;
      if (prop === 'querySelectorAll') return () => [];
      if (prop === 'getContext') return () => null;
      if (prop === 'closest') return () => null;
      if (prop === 'focus' || prop === 'blur' || prop === 'click' || prop === 'remove' ||
          prop === 'appendChild' || prop === 'append' || prop === 'prepend' ||
          prop === 'insertBefore' || prop === 'removeChild' || prop === 'replaceChildren' ||
          prop === 'setAttribute' || prop === 'removeAttribute' || prop === 'addEventListener' ||
          prop === 'removeEventListener' || prop === 'setRangeText' ||
          prop === 'setSelectionRange' || prop === 'scrollIntoView' || prop === 'dispatchEvent') {
        return () => makeStubElement();
      }
      if (prop === 'getAttribute') return () => null;
      if (prop === 'hasAttribute') return () => false;
      if (prop === 'parentElement' || prop === 'parentNode' || prop === 'firstChild' ||
          prop === 'lastChild' || prop === 'nextSibling' || prop === 'previousSibling' ||
          prop === 'nextElementSibling' || prop === 'ownerDocument') return null;
      if (prop === Symbol.toPrimitive) return () => '';
      if (typeof prop === 'symbol') return undefined;
      if (target[prop] !== undefined) return target[prop];
      // Nieznana właściwość: zwróć no-op funkcję (najbezpieczniejsze dla łańcuchów wywołań)
      return () => makeStubElement();
    },
    set(target, prop, val) {
      if (prop === 'value' || prop === 'textContent' || prop === 'innerHTML' || prop === 'outerHTML') {
        target['__' + String(prop)] = val;
      } else {
        target[prop] = val;
      }
      return true;
    }
  });
}

function buildSandbox() {
  const storage = {};
  const sandbox = {
    console,
    setTimeout: () => 0,
    clearTimeout: () => {},
    setInterval: () => 0,
    clearInterval: () => {},
    requestAnimationFrame: () => 0,
    cancelAnimationFrame: () => {},
    localStorage: {
      getItem: k => (k in storage ? storage[k] : null),
      setItem: (k, v) => { storage[k] = String(v); },
      removeItem: k => { delete storage[k]; }
    },
    navigator: { platform: 'test', userAgent: 'node-test', clipboard: { writeText: () => Promise.resolve() } },
    location: { href: 'about:test', search: '', hash: '' },
    history: { replaceState: () => {}, pushState: () => {} },
    document: null,
    window: null,
    alert: () => {},
    confirm: () => true,
    prompt: () => null,
    Blob: function Blob() {},
    URL: { createObjectURL: () => 'blob:test', revokeObjectURL: () => {} },
    FileReader: function FileReader() { this.readAsText = () => {}; },
    getComputedStyle: () => new Proxy({}, { get: () => '' }),
    matchMedia: () => ({ matches: false, addEventListener: () => {}, addListener: () => {} }),
    performance: { now: () => Date.now() },
    addEventListener: () => {},
    removeEventListener: () => {},
    dispatchEvent: () => true,
    innerWidth: 1920,
    innerHeight: 1080,
    devicePixelRatio: 2,
    scrollTo: () => {},
    getSelection: () => ({ removeAllRanges: () => {}, addRange: () => {}, rangeCount: 0 }),
    ResizeObserver: function ResizeObserver() { this.observe = () => {}; this.disconnect = () => {}; this.unobserve = () => {}; },
    MutationObserver: function MutationObserver() { this.observe = () => {}; this.disconnect = () => {}; },
    __qed_test: null
  };
  const doc = {
    getElementById: () => makeStubElement(),
    querySelector: () => makeStubElement(),
    querySelectorAll: () => [],
    createElement: () => makeStubElement(),
    createElementNS: () => makeStubElement(),
    createDocumentFragment: () => makeStubElement(),
    createTextNode: () => makeStubElement(),
    addEventListener: () => {},
    removeEventListener: () => {},
    body: makeStubElement(),
    documentElement: makeStubElement(),
    head: makeStubElement(),
    activeElement: null,
    caretRangeFromPoint: () => null,
    fonts: { ready: Promise.resolve() },
    hidden: false,
    visibilityState: 'visible'
  };
  sandbox.document = doc;
  sandbox.window = sandbox;
  sandbox.globalThis = sandbox;
  return sandbox;
}

// ============================================================
// Ekstrakcja i uruchomienie IIFE edytora z wstrzykniętym hookiem
// ============================================================
function loadEditorInternals(htmlPath) {
  const html = fs.readFileSync(htmlPath, 'utf8');
  const open = html.indexOf('<script>');
  const close = html.indexOf('</script>', open);
  if (open === -1 || close === -1) throw new Error('Nie znaleziono bloku <script> w ' + htmlPath);
  let script = html.slice(open + '<script>'.length, close);

  // Wstrzyknij hook testowy tuż przed zamknięciem IIFE — tylko w pamięci,
  // plik produktu pozostaje nietknięty.
  const tail = '})();';
  const tailIdx = script.lastIndexOf(tail);
  if (tailIdx === -1) throw new Error('Nie znaleziono zamknięcia IIFE — zmieniła się struktura pliku?');
  const hook = `
  ;globalThis.__qed_test = {
    parse: typeof parse === 'function' ? parse : null,
    lint: typeof lint === 'function' ? lint : null,
    layoutFlow: typeof layoutFlow === 'function' ? layoutFlow : null,
    findReferencedStages: typeof findReferencedStages === 'function' ? findReferencedStages : null,
    findDefinedStages: typeof findDefinedStages === 'function' ? findDefinedStages : null
  };
  `;
  script = script.slice(0, tailIdx) + hook + script.slice(tailIdx);

  const sandbox = buildSandbox();
  vm.createContext(sandbox);
  try {
    vm.runInContext(script, sandbox, { filename: path.basename(htmlPath), timeout: 30000 });
  } catch (e) {
    // Inicjalizacja UI może się wywalić na stubie DOM PO zdefiniowaniu funkcji.
    // Jeśli hook zdążył się ustawić — kontynuujemy; jeśli nie — to realny błąd.
    if (!sandbox.__qed_test || !sandbox.__qed_test.parse) {
      throw new Error('Skrypt edytora nie doszedł do hooka testowego: ' + e.message);
    }
    console.log('  (uwaga: inicjalizacja UI rzuciła po zdefiniowaniu funkcji — ignoruję: ' + e.message + ')');
  }
  if (!sandbox.__qed_test || !sandbox.__qed_test.parse || !sandbox.__qed_test.lint) {
    throw new Error('Hook testowy nie wyeksportował parse/lint.');
  }
  return sandbox.__qed_test;
}


module.exports = { loadEditorInternals };
