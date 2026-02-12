
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer over time.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.16` |
| **Date** | 2026-02-12 22:44:24 |
| **RMSE** | **0.043946** |

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
    x-axis ["b.14", "b.15", "b.16"]
    y-axis "RMSE" 0 --> 0.0527
    line [0.012064, 0.041607, 0.043946]
```

---
*Last updated by GitHub Actions on 2026-02-12 22:44:24.*
