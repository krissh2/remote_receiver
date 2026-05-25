# Run 02 — Bez uztvērēja ģeometrijas

> ⚠ AUTOMĀTISKI ĢENERĒTS no run_02_minimal_geometry artefaktiem.
> Faktu daļas aizpildītas; aizpildi narratīvu sadaļās, kas iezīmētas ar «TODO».

## Mērķis

«TODO: viena rindkopa — kāpēc šis run eksistē, ko izņēma/atstāja un kāpēc.»

## Konfigurācija

- Fīču skaits: **28**
- Modeļi: RandomForest + IsolationForest; GroupKFold (5-fold), grupa = session_id
- class_weight = 'balanced'

**Izņemtās fīčas (3):** distance_to_receiver, bearing_to_receiver, elevation_angle_to_receiver

## Rezultāti (5-fold GroupKFold)

| Metrika           | Vērtība (mean ± std) |
|-------------------|----------------------|
| Accuracy          | 0.9991 ± 0.0007 |
| F1 (viltots)      | 0.9995 ± 0.0004 |
| precision_spoofed | 0.9994 ± 0.0004 |
| recall_spoofed    | 0.9996 ± 0.0006 |
| F1 (reāls)        | 0.9883 ± 0.0109 |
| Precīzija (reāls) | 0.9892 ± 0.0155 |
| Atsaukums (reāls) | 0.9876 ± 0.0118 |
| ROC-AUC           | 1.0000 ± 0.0000 |
| train_time_s      | 8.8800 ± 1.0694 |

### Sajaukšanas matrica

```
Apvienotā confusion matrix (5-fold CV):

                  Prognoze
                  Real    Spoofed
  Patiesi Real:   44,174     515
  Patiesi Spoof:     354  916,669

              precision    recall  f1-score   support

        real       0.99      0.99      0.99     44689
     spoofed       1.00      1.00      1.00    917023

    accuracy                           1.00    961712
   macro avg       1.00      0.99      0.99    961712
weighted avg       1.00      1.00      1.00    961712
```

## Top fīčas (pēc svarīguma)

1. uas_dt_s — 0.2876
2. duplicate_position_flag — 0.1043
3. trajectory_smoothness — 0.0758
4. uas_speed_mps — 0.0698
5. speed_variance_w5 — 0.0584
6. tortuosity_w5 — 0.0511
7. uas_3d_speed_mps — 0.0506
8. straight_line_ratio — 0.0446

## Secinājums

«TODO: ko šis run pierāda. Salīdzinājums ar citiem run, ja attiecas.»
