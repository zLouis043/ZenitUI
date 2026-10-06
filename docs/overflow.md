# Modello di overflow

## Principio

Il tipo di contenitore (`LayoutType`) e il comportamento di overflow
(`overflow-x/y`) sono **assi ortogonali**. Un `Layout` qualunque può essere
scroll container; una `ScrollView` è solo uno zucchero che preconfigura
`overflow: scroll`.

Questo replica il modello HTML-CSS: `display` e `overflow` sono proprietà
indipendenti sullo stesso box.

## Valori

| Valore    | Significato                                                    |
|-----------|----------------------------------------------------------------|
| `visible` | Default. Il contenuto trabocca, nessun clip.                   |
| `hidden`  | Clip + scroll container silenzioso (scrollTo* funziona, no UI).|
| `scroll`  | Clip + scroll container + input utente + scrollbar sempre.     |
| `auto`    | Come `scroll` ma la scrollbar appare solo se maxScroll > 0.     |

## Regola del computed value

Se `overflow-x: visible` e `overflow-y != visible` → `overflow-x` diventa `auto`.
Viceversa simmetrico. Applicata in `ComputedStyle::from`.

## Comportamento per fase

| | measure | arrange | clip | input | scrollbar |
|---|---|---|---|---|---|
| visible | normale | normale | no | no | no |
| hidden  | normale | normale | sì | no | no |
| scroll  | unlimit main | offset main | sì | sì | sempre |
| auto    | normale | offset se serve | sì | sì | se max>0 |

Nota: "unlimit main" solo per `scroll`; `auto` misura normale e scopre
`maxScroll` a valle. Questo è il costo di `auto`.

## Stato

`Layout` possiede uno `ScrollState` (struct):

    struct ScrollState {
        Vec2 offset{0,0};
        Vec2 maxScroll{0,0};
        Vec2 velocity{0,0};
        float dragStartMouse{0};
        Vec2 dragStartOffset{0};
        // metodi: adjustSpace, tickInput, clamp, drawScrollbar, ...
    };

Il comportamento è attivato se `overflowX/y != visible`.

## ScrollView

    class ScrollView : public TLayout<ScrollView> {
        ScrollView(LayoutType t = LayoutType::Vertical) : TLayout<ScrollView>(t) {
            inlineBase.overflowX = Overflow::Scroll;
            inlineBase.overflowY = Overflow::Scroll;
        }
    };

Nessun override di arrange/update/draw. È solo zucchero.

## Cosa NON facciamo (e perché)

- **overscroll-behavior: auto (propaga al genitore)**: rimandato.
  Default = `contain` (blocca al bordo), come oggi.
- **scroll-snap**: fuori scope.
- **sticky positioning**: fuori scope.
- **scrollbar stylabile via ::part**: rimandato a una iterazione
  separata, quando il modello base è stabile.

## Roadmap

- **Fase 1** — Refactor puro (zero cambi di comportamento):
  split di `Layout::update`, split di `Layout::arrangeInto`,
  estrazione di `ScrollState` da `ScrollView`.
- **Fase 2** — `ScrollState` vive in `Layout`. `overflow` singolo
  pilota il comportamento. `ScrollView` diventa zucchero.
- **Fase 3** — Per-asse (`overflow-x/y`), regola del computed value,
  `Auto` operativo.