# Path Tracer Benchmark Dashboard

## Summary
| Mode | Final RMSE | Total Time | Steps |
|---|---|---|---|
| **MT** | 1.5682 | 1.01s | 2 |
| **ST** | 1.5682 | 2.96s | 2 |


## Historical Trend
```mermaid
xychart-beta
    title "Historical Performance (RMSE)"
    x-axis ["b.82", "b.84"]
    y-axis "RMSE" 0 --> 1.8818
    line [1.5682, 1.5682]
    line [1.5682, 1.5682]


    Legend: Line 1: **MT** | Line 2: **ST**

## Latest Render Gallery
| MT | ST |
| :---: | :---: |
| ![ mt ](renderings/latest-mt.png) | ![ st ](renderings/latest-st.png) |

## Convergence Comparison
```mermaid
xychart-beta
    title "RMSE Convergence Over Time"
    x-axis ["0.0s", "0.3s", "0.5s", "0.8s", "1.1s", "1.3s", "1.6s", "1.9s", "2.2s", "2.4s", "2.7s", "3.0s"]
    y-axis "RMSE" 0 --> 2.4384
    line [2.2167, 2.2167, 2.2167, 2.2167, 1.5682, null, null, null, null, null, null, null]
    line [2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 1.5682]

```

---
*Last updated: 2026-02-18T16:11:33.552643 (Commit: cca45171)*
