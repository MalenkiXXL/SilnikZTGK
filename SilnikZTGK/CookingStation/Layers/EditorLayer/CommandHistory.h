#pragma once
#include "Command.h"
#include <vector>
#include <memory>

class CommandHistory {
public:
	CommandHistory () : m_CurrentCommandIndex(-1) {}

	// wykonuje komende i dodaje j¹ do historii
	void ExecuteCommand(std::unique_ptr<Command> command) {
		// dodanie usuwanie poprzedniej akcji po tym jak zrobilismy undo i cos nowego
		if (m_CurrentCommandIndex < (int)m_Commands.size() - 1) {
			m_Commands.erase(m_Commands.begin() + m_CurrentCommandIndex + 1, m_Commands.end());
		}

		command->Execute();
		m_Commands.push_back(std::move(command));
		m_CurrentCommandIndex++;
	}

    void Undo() {
        if (m_CurrentCommandIndex >= 0) {
            m_Commands[m_CurrentCommandIndex]->Undo();
            m_CurrentCommandIndex--;
        }
    }

    void Redo() {
        if (m_CurrentCommandIndex < (int)m_Commands.size() - 1) {
            m_CurrentCommandIndex++;
            m_Commands[m_CurrentCommandIndex]->Execute();
        }
    }

    void Clear() {
        m_Commands.clear();
        m_CurrentCommandIndex = -1;
    }

private:
	std::vector<std::unique_ptr<Command>> m_Commands;
	int m_CurrentCommandIndex;
};