# Run 01 — Baseline (visas fīčas)

> ⚠ AUTOMĀTISKI ĢENERĒTS no run_01_baseline artefaktiem.
> Faktu daļas aizpildītas; aizpildi narratīvu sadaļās, kas iezīmētas ar «TODO».

## Mērķis

«TODO: viena rindkopa — kāpēc šis run eksistē, ko izņēma/atstāja un kāpēc.»

## Konfigurācija

- Fīču skaits: **31**
- Modeļi: RandomForest + IsolationForest; GroupKFold (5-fold), grupa = session_id
- class_weight = 'balanced'

Nav izņemtu fīču (pilns 31-fiicu komplekts).

## Rezultāti (5-fold GroupKFold)

| Metrika           | Vērtība (mean ± std) |
|-------------------|----------------------|
| Accuracy          | 0.9995 ± 0.0009 |
| F1 (viltots)      | 0.9998 ± 0.0005 |
| precision_spoofed | 0.9995 ± 0.0010 |
| recall_spoofed    | 1.0000 ± 0.0000 |
| F1 (reāls)        | 0.9929 ± 0.0139 |
| Precīzija (reāls) | 1.0000 ± 0.0001 |
| Atsaukums (reāls) | 0.9863 ± 0.0271 |
| ROC-AUC           | 1.0000 ± 0.0000 |
| train_time_s      | 5.5493 ± 0.5709 |

### Sajaukšanas matrica

```
Apvienotā confusion matrix (5-fold CV):

                  Prognoze
                  Real    Spoofed
  Patiesi Real:   44,239     450
  Patiesi Spoof:       1  917,022

              precision    recall  f1-score   support

        real       1.00      0.99      0.99     44689
     spoofed       1.00      1.00      1.00    917023

    accuracy                           1.00    961712
   macro avg       1.00      0.99      1.00    961712
weighted avg       1.00      1.00      1.00    961712
```

## Top fīčas (pēc svarīguma)

1. distance_to_receiver — 0.2314
2. elevation_angle_to_receiver — 0.1770
3. uas_dt_s — 0.1593
4. bearing_to_receiver — 0.1327
5. uas_3d_speed_mps — 0.0631
6. speed_variance_w5 — 0.0400
7. uas_step_distance — 0.0248
8. tortuosity_w5 — 0.0243

## Secinājums

«TODO: ko šis run pierāda. Salīdzinājums ar citiem run, ja attiecas.»
