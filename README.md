
# Benchmark Dashboard

This dashboard tracks the image quality performance (RMSE) of the renderer.

| Metric | Latest Value |
|--------|--------------|
| **Version** | `0.1.0-build.54` |
| **Date** | 2026-02-18T02:33:39.821868 |
| **Final RMSE** | **2.0441** |

## Performance Trend
```mermaid
xychart-beta
    title "RMSE Trend"
    x-axis ["b.32", "b.33", "b.34", "b.35", "b.36", "b.37", "b.39", "b.40", "b.41", "b.42", "b.43", "b.45", "b.44", "b.48", "b.49", "b.50", "b.51", "b.52", "b.53", "b.54"]
    y-axis "RMSE" 0 --> 2.4529
    line [0.2549, 0.4864, 0.0098, 0.4489, 0.7142, 0.2079, 0.1763, 0.8557, 0.7071, 0.749, 0.5226, 0.677836, 0.677984, 0.678, 0.678, 0.678, 0.6778, 0.6796, 0.6796, 2.0441]
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
    y-axis "RMSE" 0 --> 3.2241
    line [2.931, 2.4755, 2.3084, 2.2178, 2.1615, 2.1229, 2.0947, 2.074, 2.0581, 2.0441]
```

> This graph shows how the error decreased across 10 rendering steps.

---
*Last updated by GitHub Actions on 2026-02-18T02:33:39.821868.*
