# Run 04 — Bez laika zīmoga, ar operatora ģeometriju atpakaļ, GATAVAIS REZULTĀTS

> ⚠ AUTOMĀTISKI ĢENERĒTS no run_04_no_artifacts artefaktiem.
> Faktu daļas aizpildītas; aizpildi narratīvu sadaļās, kas iezīmētas ar «TODO».

## Mērķis

«TODO: viena rindkopa — kāpēc šis run eksistē, ko izņēma/atstāja un kāpēc.»

## Konfigurācija

- Fīču skaits: **25**
- Modeļi: RandomForest + IsolationForest; GroupKFold (5-fold), grupa = session_id
- class_weight = 'balanced'

**Izņemtās fīčas (6):** uas_dt_s, distance_to_receiver, bearing_to_receiver, elevation_angle_to_receiver, dt_anomaly_flag, timestamp_seq_flag

## Rezultāti (5-fold GroupKFold)

| Metrika           | Vērtība (mean ± std) |
|-------------------|----------------------|
| Accuracy          | 0.9981 ± 0.0013 |
| F1 (viltots)      | 0.9990 ± 0.0007 |
| precision_spoofed | 0.9982 ± 0.0015 |
| recall_spoofed    | 0.9998 ± 0.0001 |
| F1 (reāls)        | 0.9787 ± 0.0159 |
| Precīzija (reāls) | 0.9939 ± 0.0051 |
| Atsaukums (reāls) | 0.9645 ± 0.0328 |
| ROC-AUC           | 0.9994 ± 0.0006 |
| train_time_s      | 8.3476 ± 0.3985 |

### Sajaukšanas matrica

```
Apvienotā confusion matrix (5-fold CV):

                  Prognoze
                  Real    Spoofed
  Patiesi Real:   43,058   1,631
  Patiesi Spoof:     164  916,859

              precision    recall  f1-score   support

        real       1.00      0.96      0.98     44689
     spoofed       1.00      1.00      1.00    917023

    accuracy                           1.00    961712
   macro avg       1.00      0.98      0.99    961712
weighted avg       1.00      1.00      1.00    961712
```

## Top fīčas (pēc svarīguma)

1. uas_3d_speed_mps — 0.1484
2. duplicate_position_flag — 0.1060
3. speed_variance_w5 — 0.0962
4. dist_to_operator — 0.0824
5. trajectory_smoothness — 0.0807
6. uas_speed_mps — 0.0715
7. tortuosity_w5 — 0.0581
8. uas_step_distance — 0.0545

## Secinājums

«TODO: ko šis run pierāda. Salīdzinājums ar citiem run, ja attiecas.»
