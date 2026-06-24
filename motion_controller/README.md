# PTZ control widget for Qt

`PtzControlWidget` draws the round eight-direction VISCA/PTZ panel with a center stop button.

## CMake

```cmake
target_sources(your_target PRIVATE
    qt/PtzControlWidget.cpp
    qt/PtzControlWidget.h
)
```

## Usage

```cpp
#include "PtzControlWidget.h"

auto *ptz = new PtzControlWidget(this);
layout->addWidget(ptz);

connect(ptz, &PtzControlWidget::pressed, this, [this](PtzControlWidget::Direction direction) {
    // Start pan/tilt movement here.
});

connect(ptz, &PtzControlWidget::released, this, [this](PtzControlWidget::Direction direction) {
    // Stop movement here.
});

connect(ptz, &PtzControlWidget::clicked, this, [this](PtzControlWidget::Direction direction) {
    if (direction == PtzControlWidget::Stop) {
        // Explicit stop click.
    }
});
```

## Qt Designer

1. Add a plain `QWidget` where the PTZ control should be.
2. Right-click it and choose `Promote to...`.
3. Promoted class name: `PtzControlWidget`.
4. Header file: `qt/PtzControlWidget.h`.
5. Click `Add`, then `Promote`.

