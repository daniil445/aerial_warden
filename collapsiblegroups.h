#pragma once
#include <QEvent>
#include <QGroupBox>
#include <QLayout>
#include <QPointer>
#include <QStyle>
#include <QStyleOptionGroupBox>
#include <memory>

// Application-wide behavior for checkable groups defined in Designer.
class CollapsibleGroups : public QObject {
public:
    explicit CollapsibleGroups(QObject *parent = nullptr) : QObject(parent) {}
protected:
    bool eventFilter(QObject *object, QEvent *event) override {
        if (event->type() == QEvent::Show) {
            if (auto *widget = qobject_cast<QWidget*>(object)) {
                if (auto *group = qobject_cast<QGroupBox*>(widget)) bind(group);
                if (widget->isWindow())
                    for (auto *group : widget->findChildren<QGroupBox*>()) bind(group);
            }
        }
        return false;
    }
private:
    struct State {
        bool collapsed = false;
        QSize minimumSize, maximumSize;
        QSize expandedSize;
        QSizePolicy policy;
        QLayout::SizeConstraint constraint = QLayout::SetDefaultConstraint;
        QList<QPair<QPointer<QWidget>, bool>> children;
    };
    static void bind(QGroupBox *group) {
        if (!group->isCheckable() || group->property("collapseBehaviorInstalled").toBool()) return;
        group->setProperty("collapseBehaviorInstalled", true);
        auto state = std::make_shared<State>();
        const auto apply = [group, state](bool checked) {
            if (!checked && !state->collapsed) {
                state->collapsed = true;
                state->expandedSize = group->size();
                state->minimumSize = group->minimumSize();
                state->maximumSize = group->maximumSize();
                state->policy = group->sizePolicy();
                if (group->layout()) {
                    state->constraint = group->layout()->sizeConstraint();
                    group->layout()->setSizeConstraint(QLayout::SetNoConstraint);
                }
                for (auto *child : group->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
                    state->children.append({child, child->isHidden()});
                    child->hide();
                }
                const int height = qMax(group->fontMetrics().height(),
                        group->style()->pixelMetric(QStyle::PM_IndicatorHeight, nullptr, group)) + 16;
                const int indicatorWidth = group->style()->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, group);
                const int spacing = group->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, nullptr, group);
                const int titleWidth = group->fontMetrics().size(Qt::TextShowMnemonic, group->title()).width();
                group->setFixedSize(titleWidth + indicatorWidth + spacing + 24, height);
                group->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            } else if (checked && state->collapsed) {
                state->collapsed = false;
                group->setMinimumSize(state->minimumSize);
                group->setMaximumSize(state->maximumSize);
                group->setSizePolicy(state->policy);
                for (const auto &child : state->children)
                    if (child.first) child.first->setVisible(!child.second);
                state->children.clear();
                if (group->layout()) group->layout()->setSizeConstraint(state->constraint);
                // Floating overlay panels have no parent layout to resize them.
                // Restoring only min/max constraints leaves them at header size.
                if (group->layout()) {
                    group->layout()->invalidate();
                    group->layout()->activate();
                }
                const QSize expanded = state->expandedSize.expandedTo(group->sizeHint())
                        .expandedTo(group->minimumSize()).boundedTo(group->maximumSize());
                group->resize(expanded);
            }
            if (group->layout()) group->layout()->invalidate();
            group->updateGeometry();
        };
        QObject::connect(group, &QGroupBox::toggled, group, apply);
        apply(group->isChecked());
    }
};
