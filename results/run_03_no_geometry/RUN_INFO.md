# Run 03 — Bez ģeometrijas (eksploratīvs ablation)

**Komanda:**
```
python3 scripts/train_models.py prepared_dataset.csv \
    --run-dir runs/run_03_no_geometry --no-geometry
```

## Mērķis

Eksploratīvs eksperiments: izņemt VISA veida ģeometriju (gan uztvērēja, gan
operatora), atstājot RSSI, un pārbaudīt, vai modelis sāk balstīties uz RSSI.
NAV thesis galvenais modelis (operatora pozīcija ir leģitīms signāls) — kalpo,
lai pārbaudītu detektora robustumu un RSSI lomu.

## Konfigurācija

- Fīču skaits: **24** (31 mīnus 7 ģeometrijas fīčas)
- Modeļi: RandomForest + IsolationForest; GroupKFold (5-fold), grupa = session_id
- class_weight = 'balanced'

### Izņemtās fīčas (7)

Uztvērēja ģeometrija: distance_to_receiver, bearing_to_receiver,
bearing_to_receiver_rate, elevation_angle_to_receiver
Operatora ģeometrija: dist_to_operator, bearing_to_operator, operator_uas_bearing

## Rezultāti (5-fold GroupKFold, mean ± std)

| Metrika           | Vērtība           |
|-------------------|-------------------|
| ROC-AUC           | 1.0000 ± 0.0000   |
| F1 (reāls)        | 0.9874 ± 0.0126   |
| Precīzija (reāls) | 0.9806 ± 0.0227   |
| Atsaukums (reāls) | 0.9945 ± 0.0034   |
| F1 (viltots)      | 0.9995 ± 0.0004   |
| Accuracy          | 0.9991 ± 0.0009   |

### Sajaukšanas matrica

```
                  Prognoze
                  Reāls    Viltots
  Patiesi Reāls:  44,441      248
  Patiesi Viltots:   618  916,405
```

FP = 248 | FN = 618

## GALVENAIS SECINĀJUMS — RSSI loma

Jautājums bija: vai, izņemot visu ģeometriju, modelis sāk balstīties uz RSSI?
**Atbilde: nē.** RSSI ierindojas tikai **9. vietā ar svarīgumu 0.0357** —
praktiski tāpat kā run_02 (0.033). Augšā paliek uzvedības pazīmes:

1. uas_dt_s — 0.294
2. duplicate_position_flag — 0.113
3. uas_3d_speed_mps — 0.093
4. speed_variance_w5 — 0.066
5. straight_line_ratio — 0.062

Tas ir spēcīgs robustuma pierādījums: pat ar pieejamu RSSI modelis to NEIZVĒLAS
kā galveno pazīmi, bet balstās uz dronu kustību un trajektoriju.

PIEZĪME par RSSI interpretāciju: tavā uzņemšanā spoofer fiziski atradās tuvu
uztvērējam, reālie droni — tālu. Tāpēc RSSI satur uzņemšanas-specifisku
informāciju. Tas, ka modelis to NEIZMANTO, ir labi — detektors nepaļaujas uz
artefaktu un labāk vispārinās. (Sk. arī rssi-only eksperimentu, ja palaists.)

## Salīdzinājums starp run

| Run | Fīčas | ROC-AUC | F1 real | FP  | FN  | Top fīča            |
|-----|-------|---------|---------|-----|-----|---------------------|
| 01  | 31    | 1.0000  | 0.9929  | 450 | 1   | distance_to_receiver|
| 02  | 28    | 1.0000  | 0.9883  | 515 | 354 | uas_dt_s            |
| 03  | 24    | 1.0000  | 0.9874  | 248 | 618 | uas_dt_s            |

Veiktspēja gandrīz nemainās, izņemot arvien vairāk ģeometrijas — detektors ir
robusts pret uzņemšanas-ģeometrijas noņemšanu.
