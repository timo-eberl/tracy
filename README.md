
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer over time.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.29` |
| **Date** | 2026-02-16T14:30:52.037853 |
| **RMSE** | **0.4436** |

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
    x-axis ["b.14", "b.15", "b.16", "b.25", "b.26", "b.29"]
    y-axis "RMSE" 0 --> 0.9352
    line [0.012064, 0.041607, 0.043946, 0.7793, 0.1888, 0.4436]
```

---
*Last updated by GitHub Actions on 2026-02-16T14:30:52.037853.*
