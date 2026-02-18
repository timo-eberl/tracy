
# Path Tracer Benchmark Dashboard

## Latest Run Summary
| Mode | Final RMSE | Total Time |
|---|---|---|
| **MT** | 1.4381 | 13.57s |
| **ST** | 1.4381 | 40.51s |


## Historical Performance (Per Mode)
```mermaid
xychart-beta
    title "Historical Trend (Forward Filled)"
    x-axis ["b.61", "b.62"]
    y-axis "RMSE" 0 --> 1.7257
    line [1.4381, 1.4381]
    line [1.4381, 1.4381]
```

## Latest Render Gallery
| MT | ST |
| :---: | :---: |
| ![ mt ](renderings/latest-mt.png) | ![ st ](renderings/latest-st.png) |

## Convergence Comparison (Time-based)
```mermaid
xychart-beta
    title "RMSE vs Time (Resampled)"
    x-axis ["\"0.0s\"", "\"4.5s\"", "\"9.0s\"", "\"13.5s\"", "\"18.0s\"", "\"22.5s\"", "\"27.0s\"", "\"31.5s\"", "\"36.01s\"", "\"40.51s\""]
    y-axis "RMSE" 0 --> 2.2307
    line [2.0279, 2.0279, 2.0279, 2.0279, 1.4381, 1.4381, 1.4381, 1.4381, 1.4381, 1.4381]
    line [2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 2.0279, 1.4381]

```

---
*Last updated: 2026-02-18T13:09:46.086690*
