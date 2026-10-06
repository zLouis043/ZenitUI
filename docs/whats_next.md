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