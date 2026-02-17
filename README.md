
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.48` |
| **Date** | 2026-02-17T16:27:48.090595 |
| **Final RMSE** | **0.678** |

## Performance Trend
```mermaid
xychart-beta
    title "Historical RMSE Trend (Last 20 Builds)"
    x-axis ["b.16", "b.25", "b.26", "b.29", "b.30", "b.31", "b.32", "b.33", "b.34", "b.35", "b.36", "b.37", "b.39", "b.40", "b.41", "b.42", "b.43", "b.45", "b.44", "b.48"]
    y-axis "RMSE" 0 --> 1.1561
    line [0.043946, 0.7793, 0.1888, 0.4436, 0.9634, 0.2812, 0.2549, 0.4864, 0.0098, 0.4489, 0.7142, 0.2079, 0.1763, 0.8557, 0.7071, 0.749, 0.5226, 0.677836, 0.677984, 0.678]
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
    title "Convergence Rate (10 Total Steps)"
    x-axis ['1', '2', '3', '4', '5', '6', '7', '8', '9', '10']
    y-axis "RMSE" 0 --> 2.2378
    line [2.0344, 1.4459, 1.1882, 1.0361, 0.929, 0.8525, 0.7951, 0.7476, 0.7095, 0.678]
```

> This graph shows how the error decreased across 10 rendering steps.

---
*Last updated by GitHub Actions on 2026-02-17T16:27:48.090595.*
