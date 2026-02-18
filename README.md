
# Path Tracer Benchmark Dashboard

## Latest Run Summary
| Mode | Final RMSE |
|---|---|
| **MT** | 1.4381 |
| **ST** | 1.4381 |


## Historical Performance (Per Mode)
```mermaid
xychart-beta
    title "RMSE Trend per Mode (Forward Filled)"
    x-axis ["b.61", "b.62"]
    y-axis "RMSE" 0 --> 1.7257
    line [1.4381, 1.4381]
    line [1.4381, 1.4381]
```

## Latest Render Gallery
| MT | ST |
| :---: | :---: |
| ![ mt ](renderings/latest-mt.png) | ![ st ](renderings/latest-st.png) |

## Convergence Comparison
```mermaid
xychart-beta
    title "Convergence Rate (Current Run)"
    x-axis ["1", "2"]
    y-axis "RMSE" 0 --> 2.2307
    line [2.0279, 1.4381]
    line [2.0279, 1.4381]

```

---
*Last updated: 2026-02-18T13:09:46.086690*
