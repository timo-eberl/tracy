
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.65` |
| **Date** | 2026-02-18T13:35:49.633351 |
| **Final RMSE** | **540f1519** |

## Performance Trend
```mermaid
xychart-beta
    title "RMSE Trend"
    x-axis ["b.61", "b.61", "b.62", "b.62", "b.65"]
    y-axis "RMSE" 0 --> 1.7257
    line [1.4381, 1.4381, 1.4381, 1.4381]
```

## Latest Render
![Latest Render](renderings/latest.png)

### Convergence Progress
```mermaid
---
config:
    theme: base
    themeVariables:
        xyChart:
            plotColorPalette: "#e67e22"
---
xychart-beta
    title "Convergence Rate"
    x-axis ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10"]
    y-axis "RMSE" 0 --> 2.4341
    line [2.2128, 1.5629, 1.2813, 1.1091, 0.9916, 0.9047, 0.8368, 0.7829, 0.7385, 0.7009]
```

> This graph shows how the error decreased across 10 rendering steps.

---
*Last updated by GitHub Actions on 2026-02-18T13:35:49.633351.*
