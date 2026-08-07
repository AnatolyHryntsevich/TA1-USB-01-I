#include <QMainWindow>

class UndoBlocker : public QObject {
public:
    explicit UndoBlocker(QMainWindow *parent = nullptr) {}

    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Z && 
                keyEvent->modifiers() == Qt::ControlModifier)
            {
                return true; // Блокируем Ctrl+Z
            }
        }
        return QObject::eventFilter(obj, event);
    }
};
