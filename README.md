
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer over time.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.37` |
| **Date** | 2026-02-16T19:41:50.458062 |
| **RMSE** | **0.2079** |

## Performance Trend
```mermaid
---
config:
    theme: base
    themeVariables:
        xyChart:
            plotColorPalette: "#2980b9"
---
xychart-beta
    title "RMSE Convergence Error (Lower is Better)"
    x-axis ["b.14", "b.15", "b.16", "b.25", "b.26", "b.29", "b.30", "b.31", "b.32", "b.33", "b.34", "b.35", "b.36", "b.37"]
    y-axis "RMSE" 0 --> 1.1561
    line [0.012064, 0.041607, 0.043946, 0.7793, 0.1888, 0.4436, 0.9634, 0.2812, 0.2549, 0.4864, 0.0098, 0.4489, 0.7142, 0.2079]
```

---
*Last updated by GitHub Actions on 2026-02-16T19:41:50.458062.*
