# Path Tracer Benchmark Dashboard

## Summary
| Mode | Final RMSE | Total Time | Steps |
|---|---|---|---|
| **MT** | 1.5682 | 1.00s | 2 |
| **ST** | 1.5682 | 3.11s | 2 |


## Historical Trend
```mermaid
xychart-beta
    title "Historical Performance (RMSE)"
    x-axis ["b.82"]
    y-axis "RMSE" 0 --> 1.8818
    line [1.5682]
    line [1.5682]

```

## Latest Render Gallery
| MT | ST |
| :---: | :---: |
| ![ mt ](renderings/latest-mt.png) | ![ st ](renderings/latest-st.png) |

## Convergence Comparison
```mermaid
xychart-beta
    title "RMSE Convergence Over Time"
    x-axis ["0.0s", "0.3s", "0.6s", "0.8s", "1.1s", "1.4s", "1.7s", "2.0s", "2.3s", "2.5s", "2.8s", "3.1s"]
    y-axis "RMSE" 0 --> 2.4384
    line [2.2167, 2.2167, 2.2167, 2.2167, 1.5682, null, null, null, null, null, null, null]
    line [2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 2.2167, 1.5682]

```

---
*Last updated: 2026-02-18T16:02:49.989979 (Commit: 674e005f)*
