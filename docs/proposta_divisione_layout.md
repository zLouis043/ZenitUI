Hai ragione, e la tua osservazione è più importante di quanto sembri. Il progetto **è nato** con l'idea di avere `Style` e `AnimPrimitives` come moduli a sé, poi `Layout` li ha inglobati perché "tanto è più comodo". Ora `Layout` è un contenitore onnivoro. La domanda vera non è "come lo scomponiamo", ma "**quale astrazione vogliamo che `Layout` sia**".

Prima di scrivere una riga, ragioniamo su questo.

## Cosa dovrebbe essere `Layout`

Tre modelli possibili, in ordine crescente di "scomposizione":

**Modello A — Layout come contenitore di stato.** Layout ha membri-sottosistemi (`style_`, `anim_`, `scroll_`, ...) come struct con dati + metodi, ma i metodi pubblici di Layout restano il punto d'ingresso. I file sono separati, le classi no.

**Modello B — Layout come orchestratore.** Layout ha dei **sistemi** (`StyleResolver`, `AnimationPlayer`, `LayoutEngine`, `Renderer`, `HitTester`, `InputController`) che operano su di lui. Ogni sistema ha stato proprio e metodi che prendono `Layout*`.

**Modello C — Layout come nodo passivo.** Layout è solo dati (identità, albero, listener). Tutta la logica vive in "sistemi" esterni che girano sull'albero. È il modello di React, di Flutter (in parte), di alcuni motori.

Il modello C è il più pulito in teoria e il più disastroso in pratica per un progetto come il tuo: passi `Layout*` a tutto, hai mille getter pubblici, e ogni accesso a `currentStyle` diventa `ctx.getStyle(node)`. Non lo farei.

Il modello A è il più vicino a quello che hai già. È uno **split fisico dello stato in struct separate**, ma i metodi restano dove sono. Buono per leggibilità, ma non risolve davvero il "Dio oggetto" — la classe Layout ha ancora 3000 righe di logica.

Il modello B è quello che descrivi tu ("Style.hpp, AnimPrimitives.hpp dovevano essere sottosezioni usate da Layout"). Layout diventa orchestratore magro, i sistemi hanno responsabilità chiara.

## La mia lettura onesta

**Il modello B è quello giusto, ma va fatto a metà.** Intendo: non tutti i sottosistemi meritano di diventare oggetti separati. Alcuni sono troppo accoppiati al ciclo di Layout per guadagnarci qualcosa. Altri sono già quasi indipendenti e diventarlo è naturale.

Ecco la classificazione che farei:

### Diventano sistemi veri (stato + metodi, prendono `Layout&`)

**`StyleResolver`** — risolve la cascata, gestisce `currentStyle`/`targetStyle`/`transitionStartStyle`, ticka le transizioni, propaga l'ereditarietà. Stato: tutti gli `inline*`, `currentStyle`, `targetStyle`, `transitionStartStyle`, `transitionTimer`, `partTransitions`, `lastSnapshot`. Metodi: `resolveFor(Layout&)`, `partFor(Layout&, name)`, `onStateChanged(Layout&, state)`, `tick(Layout&, dt)`, `propagate(Layout&)`.

**Perché funziona**: la cascata è un algoritmo a sé, ben delimitato. `StyleResolver` non ha bisogno di sapere cosa fa `measure` o `draw`.

**`AnimationPlayer`** — gestisce `activeAnimations` e `activeCssAnimations`, ticka, fa sync con le regole CSS. Stato: i due vector/map. Metodi: `tickImperative(Layout&, dt)`, `tickCss(Layout&, dt)`, `syncCss(Layout&)`, `play(Layout&, name, reverse)`.

**Perché funziona**: è già oggi quasi indipendente. L'unico aggancio è che `tickImperative` scrive in `pendingTransition`, ma questo è un flag.

**`ScrollController`** — l'abbiamo già preparato. Stato: `scroll_` (ScrollState) + `scrollContentSize`. Metodi: `arrangeAdjust(Layout&, space)`, `postArrange(Layout&, space)`, `tickInput(Layout&)`, `drawScrollbar(Layout&, opacity)`.

**Perché funziona**: è indipendente, tranne per leggere `rect` e `currentStyle.overflow`.

**`InputController`** — hover/pressed/focused/checked, callback, focus management, Tab navigation. Stato: `isHovered`, `isPressed`, `isFocused`, `currentState`, `focusable`, `focusScope`, `keyboardActivates`, `lastSnapshot` (parte input). Metodi: `updateFlags(Layout&)`, `fireCallbacks(Layout&)`, `handleKeyInput(Layout&)`.

**Perché funziona**: gli stati sono una macchina a stati finiti ben definita. Le callback sono agganci.

### Restano in Layout (ma codice in file separati)

**`Geometry`** — `rect`, `measuredSize`, `subtreeDirty_`, `lastMeasureW_/H_`. Non ha senso come oggetto separato perché il 90% del codice di layout ci accede direttamente. Diventa una struct membro, e i metodi `measure`/`arrange` restano metodi di Layout ma fisicamente in `Layout.Geometry.cpp`.

**`Renderer`** — `draw`, `renderChrome`, `renderContent`, `currentTransform`. Anche questo tocca troppe cose di Layout (stile, rect, figli) per essere un oggetto separato. Resta metodo di Layout, fisicamente in `Layout.Render.cpp`.

**`HitTester`** — `hitTest`, `isStackingContext`, `getZIndex`. Idem.

**`DirtyTracker`** — `recomputeDirty`, `markInheritanceDirty`. Idem, resta in Layout.

## Perché questa divisione e non una più netta

Il criterio è: **un sottosistema diventa oggetto separato se il suo stato e la sua logica sono autosufficienti**. Se il sottosistema deve leggere 5 campi di Layout e scriverne 3, allora è Layout travestito — meglio lasciarlo metodo.

`StyleResolver` e `AnimationPlayer` sono autosufficienti perché lavorano su dati ben delimitati (regole CSS, keyframes) e producono un risultato (ComputedStyle, Style delta). `ScrollController` è autosufficiente perché riceve uno spazio e restituisce uno spazio. `InputController` è autosufficiente perché è una FSM.

`measure`/`arrange` non sono autosufficienti: leggono e scrivono di tutto. Il rendering nemmeno.

## Come apparirebbe `Layout.hpp` dopo

```cpp
class Layout : public std::enable_shared_from_this<Layout>
{
public:
    // API pubblica invariata (scrollTo, setChecked, capturePointer, ...)

    // Ciclo di vita (invariato)
    virtual Vec2 measure(float w, float h);
    virtual void arrange(Rect space);
    virtual void update(float dt, bool blocked);
    virtual void draw(float parentOpacity);

    // ... API pubblica esistente ...

protected:
    // Identità
    LayoutType type;
    std::string styleTag;
    std::vector<std::string> styleClasses;
    std::string nodeId;
    std::weak_ptr<Layout> parent;
    std::vector<std::shared_ptr<Layout>> children;

    // Sistemi (autosufficienti)
    StyleResolver    style_;
    AnimationPlayer  animations_;
    ScrollController scroll_;
    InputController  input_;

    // Geometria (dati condivisi)
    Rect  rect{0,0,0,0};
    Vec2  measuredSize{0,0};
    bool  subtreeDirty_{true};
    float lastMeasureW_{-1}, lastMeasureH_{-1};

    // Texture/shader/ecc.
    TextureHandle bgTexture;
    NineSlice bgPatchInfo;
    EffectHandle customEffect;
    bool hasShader{false};

    // ... hook virtuali (onBuild, onLayout, renderContent, ...) invariati ...
};
```

E `Layout.cpp` diventerebbe:

```cpp
void Layout::update(float dt, bool ancestorBlocked) {
    style_.resolveIfNeeded(*this);
    scroll_.resetIfOverflowChanged(*this);

    bool blocksInput = animations_.tickImperative(*this, dt);
    bool blocked = ancestorBlocked || blocksInput;

    input_.updateFlags(*this, blocked);
    input_.transitionState(*this);       // chiama style_.onStateChanged se serve

    style_.tick(*this, dt);
    animations_.tickCss(*this, dt);
    style_.propagateInheritance(*this);

    for (auto& c : children)
        c->update(dt, blocked);

    scroll_.tickInput(*this);
    input_.handleFocusInput(*this);
    input_.fireCallbacks(*this);
    cullRemovedChildren();
    recomputeDirty(...);

    if (isEnabled || updateWhenDisabled_)
        onUpdate(dt);
}
```

Molto più leggibile. Ogni riga dice cosa succede, e se vuoi il dettaglio apri il modulo.

## Il costo reale

Non ti nascondo che è un refactoring **sostanziale**:

- Tocca ~2500 righe di codice in 6-8 file.
- Ogni sistema deve essere dichiarato e inizializzato (niente di grave, ma va fatto).
- Alcune cose diventano più verbose: `scroll_.offset.y` invece di `scroll_.offset.y` (già fatto in Fase 2), `style_.currentStyle` invece di `currentStyle`.
- Serve un periodo di transizione in cui Layout ha **sia** i vecchi metodi **sia** i nuovi sistemi, con il vecchio che delega al nuovo. Poi si cancellano i vecchi.

Stima realistica: **una settimana di lavoro serio**, con la possibilità di fare il merge in 4-5 step committabili.

## Il mio consiglio operativo

Non fare il big-bang. Ecco come farei io:

**Step 0 (1 ora)** — PCH. Non c'entra col refactoring, ma ti dà un ambiente di sviluppo 5× più veloce e rende tutto il resto più sopportabile.

**Step 1 (mezza giornata)** — Split fisico `Layout.cpp` in 5 file (`.Geometry.cpp`, `.Style.cpp`, `.Anim.cpp`, `.Input.cpp`, `.Scroll.cpp`). Zero cambi di classe. Solo copia-incolla e `#include`. Verifichi che la demo funzioni. **Questa è la base igienica.**

**Step 2 (1 giorno)** — Estrai `StyleResolver` come membro di Layout. Tutti i metodi di stile diventano `style_.metodo(*this, ...)`. `Layout.hpp` sostituisce ~20 campi con un unico `StyleResolver style_`. `Layout.cpp` (o `Layout.Style.cpp`) rimane lo stesso, ma chiama il resolver.

**Step 3 (1 giorno)** — Estrai `AnimationPlayer`. Stessa ricetta.

**Step 4 (1 giorno)** — Estrai `ScrollController`. Stessa ricetta. A questo punto hai tre sistemi, e Layout è dimezzato.

**Step 5 (1 giorno)** — Estrai `InputController`. Layout è ora orchestratore.

**Step 6 (mezza giornata)** — Pulisci: rimuovi i metodi deleganti orfani, aggiorna `Debug.hpp` se accede a campi spostati, aggiorna commenti.

Ogni step è committabile da solo e non rompe niente. Se ti fermi a metà, il codice funziona comunque.

**Non fare l'opzione "in un colpo solo"**. Ci ho pensato, ma il rischio di rompere qualcosa di sottile (transizioni, focus, dirty tracking) è troppo alto per non fare step intermedi.

## Domanda per te

Prima di scrivere codice, dimmi:

1. **Sei d'accordo con la divisione "sistemi veri" vs "resta in Layout"?** (StyleResolver, AnimationPlayer, ScrollController, InputController come sistemi; geometry/rendering/hit-testing come metodi di Layout in file separati). Oppure vuoi spingere più in là, tipo `GeometrySystem` separato?

2. **Vuoi fare Step 0 (PCH) prima?** Lo consiglio, ma se preferisci attaccare direttamente il refactoring, salto.

3. **Facciamo un ramo Git separato?** Con un refactoring così grosso, `git switch -c refactor/layout-split` ti salva se qualcosa va storto.

Se sei d'accordo sul modello e sull'ordine, il primo pezzo concreto che scrivo è **Step 1**: i 5 file `.cpp` fisicamente separati con `Layout.hpp` invariato. Da lì vediamo subito se il codice respira meglio, e decidiamo se procedere con lo Step 2.