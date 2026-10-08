Sì, sono quelle cinque. Ma non hanno tutte lo stesso peso, e una in particolare **non appartiene alla filosofia HTML/CSS** che hai scelto. Te le guardo una per una in ordine di "quanto si incastrano nell'architettura attuale".

## 1. Custom properties con eredità — priorità alta, refactor reale

Questa è **la più importante** perché il tuo CSS oggi ha una falla semantica: `var(--x)` viene sostituito **a parse time** con le variabili globali dello sheet. Non c'è scoping, non c'è ereditarietà, non c'è override per sottoalbero. In CSS vero:

```css
.section { --accent: blue; }
.section .card { color: var(--accent); }   /* blu */
.other .card  { color: var(--accent); }   /* default, se definito a :root */
```

Da te oggi **non funziona**, perché la sostituzione avviene quando leggi `game.zstyle`, una volta sola, senza sapere dove la regola verrà applicata.

**Il problema tecnico**: `Style` ha già i valori **parsati** (`Opt<Value>`, `Opt<Color>`). La sostituzione deve avvenire **prima** del parse, ma **dopo** il cascade (per sapere quali var eredita il nodo). Questo rompe il modello attuale `parse → Style pronto`.

**Come lo farei in ZenitUI**:
- `Value::Term` guadagna un campo `varName` (o `Unit::VarRef`).
- Il parser, quando vede `var(--x)`, produce un termine con `varName = "--x"` invece di espandere subito.
- `Style` e `ComputedStyle` guadagnano `std::unordered_map<std::string, std::string> customProps` (valori grezzi).
- `ComputedStyle::from` eredita `customProps` dal parent, fa overlay del proprio.
- `StyleResolver::resolveFor` chiama una `substituteVars(finalStyle, inheritedCustomProps)` **prima** di `ComputedStyle::from`, che risolve tutti i termini con `varName`.

Effort: **mezza giornata**. È il prerequisito per tutto il resto (decoratori spesso usano var, media queries possono settare var per breakpoint).

## 2. Media queries — priorità media, mezza giornata

Utile per adattarsi a risoluzioni diverse o DPI diverse. In un gioco la finestra è fissa, quindi l'utilità è "quando il giocatore ridimensiona la finestra". In un tool/editor, è più utile.

**Come lo farei in ZenitUI**: 
- `@media (min-width: 800px) { ... }` diventa un blocco di regole ognuna con un `Opt<MediaQuery>`.
- `ruleMatches` fa un check rapido: `if (r.media && !evaluateMedia(*r.media)) return false;`.
- `evaluateMedia` confronta `Metrics::viewport` con la condizione parsata.

Il parser è la parte noiosa (devi tokenizzare `(min-width: 800px) and (orientation: landscape)`). La valutazione è una riga.

Effort: **mezza giornata**.

## 3. Decoratori — priorità media, ma ambito vasto

Questa è **la più "RmlUI"** delle cinque, e la più aperta. Due strade possibili:

**Minimal (che consiglio)**: una lista chiusa di decoratori built-in che CSS può attivare. `decorator: gradient(linear, #000, #fff)`, `decorator: shadow(4px, 4px, #00000080)`, `decorator: border-image(my-texture, 16 16 16 16)`. Ognuno è una funzione C++ chiamata da `renderChrome`.

**Full (RmlUI-style)**: API di registrazione, l'utente scrive i suoi decoratori. `AnimDecorator` registers decorators. Più potente ma serve un'API pubblica ben progettata, e il rendering deve esporre hook. Se il tuo `IRenderer` cresce, questa è la strada.

Effort: **4-6 ore per minimal, 1-2 giorni per full**.

Consiglio personale: implementa 3-4 decoratori built-in quando ti serviranno (gradient, shadow, border-image). Vedi se ti bastano. Solo se ti trovi a voler scrivere un decoratore custom, aggiungi l'API pubblica.

## 4. Data binding — priorità bassa, dubbia

Qui ho una posizione forte: **HTML e CSS non hanno data binding**. È React/Vue/Alpine. RmlUI lo ha perché è nato come framework web in C++, ma **è il suo pezzo meno coerente** — la community di RmlUI stessa si lamenta che `data-for` e i template sono fragili.

Se aggiungi data binding a ZenitUI, perdi la coerenza "sono un browser": diventi un ibrido. E non guadagni molto, perché puoi già scrivere:

```cpp
auto label = Label("0");
slider->onValueChanged = [label](float v) { label->setText(std::to_string((int)v)); };
```

Una soluzione leggera che **non tradisce HTML**:
- Un piccolo sistema di **Signal/Observable** (`Signal<float>`, `Signal<std::string>`, ecc.).
- Widget accettano opzionalmente un Signal al posto del valore: `Slider(signalVolume)`.
- Un `ZMarkup::bind()` che collega un attributo a un Signal per id: `Text#volume-display bind-text="volume"`.

Questo è ~300 righe, non tocca il modello CSS, e ti dà il 90% del valore del data binding senza il 90% della complessità.

Effort: **6-10 ore** per la versione mid-tier.

**Ma il mio consiglio è di non farlo adesso.** Usa ZenitUI in un progetto vero, e se ti trovi a scrivere la stessa `onValueChanged` → `setText` 30 volte, *allora* aggiungilo. Il bisogno ti dirà il design giusto.

## 5. On-demand rendering — priorità bassa, quasi mai utile in gioco

In un gioco rendering a 60 fps, il costo di `draw()` è **parte del lavoro utile** — stai renderizzando la scena comunque. Skippare la UI quando non cambia ti fa risparmiare poco, e introduce un sacco di edge case (cursor blink, animazioni CSS, portal che appaiono, hover).

Sarebbe utile in un tool standalone o in un editor, dove il redraw è dominato dalla UI. Non è il tuo caso.

Effort: **2-3 ore**, ma **non lo farei** a meno che tu non stia costruendo un editor.

---

## Come si pongono, in sintesi

| Feature | Chi la usa | Dove si incastra | Effort | Priorità |
|---|---|---|---|---|
| Custom properties | CSS | Parser + Style/ComputedStyle + resolveFor | Mezza giornata | **Alta** (fix di un bug latente) |
| Media queries | CSS | Parser + ruleMatches | Mezza giornata | Media |
| Decoratori | Rendering | Style + renderChrome + registry | 4-6h / 1-2gg | Media |
| Data binding | Logica | API fuori dal CSS (Signal + ZMarkup) | 6-10h | Bassa |
| On-demand rendering | Runtime | `main.cpp` + dirty check globale | 2-3h | Bassa |

## Il mio ordine operativo

1. **Custom properties** — è un fix di correttezza, e sblocca gli altri. Fallo appena hai tempo.
2. **Media queries** — se lavori con finestre ridimensionabili (o se vuoi un domani supportare DPI diversi), fallo. Altrimenti salta.
3. **Decoratori minimal** — quando senti il bisogno di un effetto che `::part` non copre. Non prima.
4. **Data binding** — solo se ti trovi a scrivere boilerplate reattivo tre volte. Fino ad allora, `setText` e callback bastano.
5. **On-demand rendering** — quasi mai. Salta a meno che tu non stia facendo un editor.

Il punto vero è: **nessuna di queste è urgente per usare ZenitUI adesso**. Sono tutte "aggiunte quando servono". Il custom properties è l'unica che ha un pizzico di "debito" perché il comportamento attuale non è corretto secondo lo standard HTML/CSS che hai scelto come modello. Le altre sono feature: le aggiungi se ti servono, quando ti servono.


------------------------------------------

Bene, andiamo a fondo sui due punti. Sono due problemi diversi ma legati da un tema comune: **la separazione tra astrazione e backend**.

# 1. Shader al root vs shader per-widget

La domanda è giusta perché rivela un'ambiguità che il codice attuale non risolve. Oggi `pushEffect` è naive: chiama `BeginShaderMode` e spera che vada bene. Se ci pensi, "shader al root" può significare tre cose diverse:

## Tre semantiche possibili

**A) Shader per-primitiva (inline).** Ogni `fillRect`, `drawText`, ecc. passa dal shader. Utile per: tint, hue shift, saturazione, brightness. Non serve un render target.

**B) Shader post-process (filter).** Il subtree del nodo viene disegnato su un render target, poi il shader lo legge pixel-per-pixel e lo scrive sul framebuffer. Utile per: blur, drop-shadow, distorsione, outline, glow. **Serve un render target.**

**C) Shader di compositing.** Il nodo viene disegnato normalmente, ma al momento di comporlo sul genitore passa da un blend shader. Utile per: mix-blend-mode, mask, chroma key.

Sono tre cose diverse. Il tuo `pushEffect` attuale copre solo (A), e anche male — perché Raylib non stacca, quindi due `pushEffect` annidati si sovrascrivono.

## Cosa succede se applichi uno shader al root

Con (A): il root shader tocca ogni primitiva dell'intero albero. Un widget con shader proprio fa `pushEffect(widget)` che **rimpiazza** il root shader per quel subtree. Quando il widget fa `popEffect`, Raylib non ripristina il precedente — esce dallo shader del tutto. Il root shader è perso per il resto del frame. **Composizione rotta.**

Con (B): ogni nodo con shader diventa un **confine di render target**. Il suo subtree renderizza su una texture, il shader la processa, il risultato viene composto sul genitore. Il root diventa solo un altro nodo con `filter`. **La composizione funziona naturalmente**, perché ogni livello è un layer a sé. È esattamente come funziona CSS `filter:` nei browser.

La risposta alla tua domanda è quindi: **se implementi (A) come unico modello, uno shader al root e uno per widget si rompono a vicenda. Se implementi (B) come modello principale, root e widget convivono senza problemi e il root è solo "il primo filtro della catena".**

## Il modello che consiglio

Due proprietà CSS distinte:

```css
.widget {
  effect: hueShift(0.3);        /* (A) inline: per-primitiva */
}

.card {
  filter: blur(4px) drop-shadow(2px, 2px, #00000080);  /* (B) post-process */
}
```

**`effect:`** — shader inline, applicato a ogni draw call del nodo **e dei figli** (dopo il push). Usa `pushEffect/popEffect` come oggi. Non compone: il widget più vicino vince. Utile per trasformazioni di colore.

**`filter:`** — lista di effetti post-process. Se il nodo ha `filter`, il suo subtree viene disegnato su un render target, ogni filtro viene applicato in sequenza, il risultato viene composto sul genitore. **Composizione naturalmente corretta**. Il nodo con `filter` è un "layer" a tutti gli effetti.

Il root è solo un nodo che può avere `filter`. Nessun caso speciale.

## Perché questa separazione

- `effect` è economico (nessun render target) → lo usi per micro-tweaks frequenti.
- `filter` è costoso (2N draw call invece di N) → lo usi dove serve davvero.
- Concettualmente allineati a CSS, che è il tuo modello mentale.
- Non devi "rimediare" al fatto che Raylib non stacca i shader — il modello `filter` **aggira il problema per design**.

## Cosa manca nel codice per arrivare lì

1. `RaylibAssetProvider::loadEffect(name, path)` — caricare shader da file.
2. `EffectHandle` già esiste, ma serve associarci il nome del file per il reload/debug.
3. `Style::effect` (string) e `Style::filter` (lista di `FilterRef{name, params}`).
4. Parser CSS: `effect: hueShift;` e `filter: blur(4px);`.
5. `Layout::draw`: se `filter` è non-vuoto, il nodo diventa un layer (crea target, disegna subtree, applica filtri, compone). Altrimenti comportamento attuale.
6. Per `effect`: fixare il comportamento di push/pop — il fix più semplice è **tracciare uno stack nel renderer** e fare pop che ripristina il precedente invece di spegnere tutto.

Quest'ultimo punto è importante: oggi `popEffect` fa `EndShaderMode`, che spegne tutto. Va cambiato in: se lo stack è vuoto, `EndShaderMode`, altrimenti riapplica il precedente. Un `std::vector<EffectHandle>` nel `RaylibRenderer`.

# 2. Cosa aggiungere agli hook per DPI e input alternativi

Hai ragione: raylib è solo un backend. La domanda corretta non è "cosa fa raylib" ma **"quale astrazione serve perché un backend mobile/touch/DX12/SDL possa implementarla senza rompere il framework"**.

## DPI

Il modello giusto è: **il framework lavora interamente in CSS pixel** (logical pixels). Il backend traduce in pixel fisici al momento del rendering.

Nel `IPlatform`:
```cpp
virtual float dpiScale() = 0;   // 1.0 desktop, 2.0/3.0 su retina
```

Nel `IRenderer`:
```cpp
virtual void beginFrame() = 0;   // hook per settare scale/DPI
virtual void endFrame() = 0;
```

Il renderer applica la scala nel suo `pushTransform` di base, oppure la incorpora in `fillRect`/`drawText` (meglio: applica in fase di scrittura dei vertici).

**Nel framework non cambia niente.** Tutti i `Value::resolve*` continuano a lavorare in logical pixel. `Metrics::viewport` rimane in logical pixel. Il `dpiScale` è puramente del backend.

Questo è il punto chiave: **il DPI non deve entrare nel layout**. Se entra, ogni calcolo diventa ambiguo ("questo `Px(20)` è logico o fisico?") e diventa un incubo. Tienilo fuori.

## Safe area (notch)

```cpp
struct EdgeInsets { float top, right, bottom, left; };
virtual EdgeInsets safeArea() = 0;   // inset in logical pixel
```

Esposto su `UIContext::safeArea`. Il framework può consumarlo tramite una classe CSS tipo `.safe-area-top` o una pseudo-proprietà `padding-env: safe-area-top`. Il codice utente può anche usarlo direttamente. **Non** lo applichi automaticamente al root — lascia decidere.

## Input: dal singolo pointer al multi-pointer

Questo è il pezzo più delicato. Il modello attuale è: `PointerState` singolo. Un modello mobile richiede:

**a) Multi-pointer.** `std::vector<Pointer>`. Ogni pointer ha `id` (mouse = -1, touch = 0..N), `position`, `phase` (Began/Moved/Ended/Cancelled), `pressure`, `isPrimary` (il primo pointer attivo).

**b) Hover separato dal pointer.** Mouse ha hover. Touch non ha hover — appare e agisce. Il modello attuale (`isHovered` come stato del widget) funziona per mouse ma su touch è un concetto vuoto. Va bene se "hover = false quando l'input è touch", ma serve un modo per il framework di saperlo.

**c) Gesture come layer separato.** Tap, long-press, swipe, pinch. Questi **non** vanno nel `Pointer`. Vanno in un `IGestureRecognizer` opzionale o in uno strato `GestureProcessor` sopra `UIContext`. Il framework di UI non li usa (i widget ricevono click/scroll come oggi). Il codice utente può consumarli per camera, zoom, ecc.

**d) Gamepad.** Non è un pointer. Non ha posizione. La navigazione è focus-driven. Modello: `IPlatform::gamepadState()` con direzioni + pulsanti. `InputController` esteso con una branch gamepad: `InputEvent::Navigate(Direction)` che muove il focus nell'albero (il tuo `updateTree` già fa Tab, che è quasi la stessa logica).

## Cosa cambia concretamente negli hook

**`IPlatform`:**
```cpp
virtual float dpiScale() = 0;
virtual EdgeInsets safeArea() = 0;
virtual std::vector<Pointer> pollPointers() = 0;   // era pointer()
virtual GamepadState gamepad() = 0;
```

**`PointerState`** (rinominato `Pointer`):
```cpp
struct Pointer {
    int id{-1};
    Vec2 pos;
    Phase phase;              // Began/Moved/Ended/Cancelled/Stationary
    bool isPrimary{true};
    bool hasHover{true};      // mouse/stylus: true, touch: false
    float pressure{0.0f};
    // legacy compat per widget esistenti:
    bool down, pressed, released, rightDown, rightPressed, rightReleased;
    float wheelY;
};
```

**`UIContext`:**
```cpp
std::vector<Pointer> pointers;         // tutti i pointer attivi
Pointer& primaryPointer();             // scorciatoia
float dpiScale{1.0f};
EdgeInsets safeArea;
// Il resto (topmostConsumer, pressTarget, ecc.) resta com'è oggi,
// ma lavora sul primaryPointer.
```

**`IRenderer`:**
```cpp
virtual void beginFrame();   // hook per DPI/clear
virtual void endFrame();
// o, meglio, UIContext chiama renderer->beginFrame() prima di draw()
```

**InputController**: accetta anche `GamepadState`, gestisce `Navigate(Dir)` — praticamente è un'estensione della logica Tab che hai già.

## Cosa **non** toccare

- `Value`, `resolveH/V`, `Unit` — restano in logical pixel.
- `Layout::measure/arrange` — restano in logical pixel.
- Widget — non sanno nulla di pointer multipli. Continuano a usare `isHovered`, `isPressed`, `onClick` come oggi.
- `Metrics::viewport` — resta in logical pixel.

Tutto il multi-touch, DPI, safe-area, gamepad **non entra nel modello di layout**. È tutto sopra (input) o sotto (rendering). Questa è l'unica separazione che rende il framework portabile.

# Sintesi

**Shaders:**
- Aggiungi il concetto di **layer** (nodo con `filter` → render target).
- Mantieni `effect` come shader inline con stack corretto nel renderer.
- Il root è solo un nodo con eventuale `filter`. Nessun caso speciale.
- Aggiungi `loadEffect` all'asset provider.

**Portabilità:**
- DPI è un problema del **backend**, non del layout. Esponi `dpiScale()` e lascia al renderer la conversione.
- Multi-pointer è un problema dell'**input**, non dei widget. Esponi `std::vector<Pointer>`, `primaryPointer()` per retrocompat.
- Gesture e gamepad sono **layer sopra il context**, non dentro. Non inquinano il framework di UI.

**Prossimi passi concreti, in ordine:**
1. `loadEffect` + fix dello stack `pushEffect/popEffect` (sblocca shader per-widget immediatamente, mezz'ora).
2. Modello `filter` con layer boundary (sblocca blur/drop-shadow, mezza giornata).
3. Media queries (indipendente, mezza giornata).
4. `dpiScale` + `safeArea` nel IPlatform, propagato a UIContext (mezza giornata).
5. `std::vector<Pointer>` (una giornata, tocca l'input path).

Se vuoi partire, direi **1 + 2** insieme: sono coerenti e si sbloccano a vicenda. Media queries e DPI dopo.

Quale di questi vuoi attaccare per primo?