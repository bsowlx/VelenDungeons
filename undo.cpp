#include "undo.h"

void UndoStack::push(Snapshot s) {
    stack_.push(s);
}

std::optional<Snapshot> UndoStack::pop() {
    if (stack_.empty()) return std::nullopt;
    Snapshot top = stack_.top();
    stack_.pop();
    return top;
}
