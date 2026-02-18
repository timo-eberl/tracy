# Path Tracer Benchmark Dashboard

## Summary
| Mode | Final RMSE | Total Time | Steps |
|---|---|---|---|
| **MT** | 1.5629 | 1.00s | 2 |
| **ST** | 1.5629 | 3.10s | 2 |


## Historical Trend
```mermaid
xychart-beta
    title "Historical Performance (RMSE)"
    x-axis ["b.89", "b.90", "b.92"]
    y-axis "RMSE" 0 --> 1.8755
    line [1.5629, 1.5629, 1.5629]
    line [1.5629, 1.5629, 1.5629]
```

> **Legend:** Line 1: **MT** | Line 2: **ST**

## Latest Render Gallery
| MT | ST |
| :---: | :---: |
| ![ mt ](renderings/latest-mt.png) | ![ st ](renderings/latest-st.png) |

## Convergence Comparison
![Convergence Plot](renderings/convergence.png)

---
*Last updated: 2026-02-18T16:55:10.512570 (Commit: 7936fd67)*
