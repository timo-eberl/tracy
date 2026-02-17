
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.51` |
| **Date** | 2026-02-17T17:00:44.977856 |
| **Final RMSE** | **0.6778** |

## Performance Trend
```mermaid
xychart-beta
    title "RMSE Trend"
    x-axis ["b.29", "b.30", "b.31", "b.32", "b.33", "b.34", "b.35", "b.36", "b.37", "b.39", "b.40", "b.41", "b.42", "b.43", "b.45", "b.44", "b.48", "b.49", "b.50", "b.51"]
    y-axis "RMSE" 0 --> 1.1561
    line [0.4436, 0.9634, 0.2812, 0.2549, 0.4864, 0.0098, 0.4489, 0.7142, 0.2079, 0.1763, 0.8557, 0.7071, 0.749, 0.5226, 0.677836, 0.677984, 0.678, 0.678, 0.678, 0.6778]
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
    y-axis "RMSE" 0 --> 2.2377
    line [2.0343, 1.4458, 1.1881, 1.036, 0.9288, 0.8524, 0.795, 0.7474, 0.7094, 0.6778]
```

> This graph shows how the error decreased across 10 rendering steps.

---
*Last updated by GitHub Actions on 2026-02-17T17:00:44.977856.*
