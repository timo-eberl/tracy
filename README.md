
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.56` |
| **Date** | 2026-02-18T02:42:27.405258 |
| **Final RMSE** | **0.7121** |

## Performance Trend
```mermaid
xychart-beta
    title "RMSE Trend"
    x-axis ["b.34", "b.35", "b.36", "b.37", "b.39", "b.40", "b.41", "b.42", "b.43", "b.45", "b.44", "b.48", "b.49", "b.50", "b.51", "b.52", "b.53", "b.54", "b.55", "b.56"]
    y-axis "RMSE" 0 --> 2.4529
    line [0.0098, 0.4489, 0.7142, 0.2079, 0.1763, 0.8557, 0.7071, 0.749, 0.5226, 0.677836, 0.677984, 0.678, 0.678, 0.678, 0.6778, 0.6796, 0.6796, 2.0441, 2.0441, 0.7121]
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
    y-axis "RMSE" 0 --> 2.4384
    line [2.2167, 1.5682, 1.2874, 1.1163, 0.9994, 0.9133, 0.8461, 0.7929, 0.7493, 0.7121]
```

> This graph shows how the error decreased across 10 rendering steps.

---
*Last updated by GitHub Actions on 2026-02-18T02:42:27.405258.*
