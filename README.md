
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer over time.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.30` |
| **Date** | 2026-02-16T15:50:54.410181 |
| **RMSE** | **0.9634** |

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
    x-axis ["b.14", "b.15", "b.16", "b.25", "b.26", "b.29", "b.30"]
    y-axis "RMSE" 0 --> 1.1561
    line [0.012064, 0.041607, 0.043946, 0.7793, 0.1888, 0.4436, 0.9634]
```

---
*Last updated by GitHub Actions on 2026-02-16T15:50:54.410181.*
