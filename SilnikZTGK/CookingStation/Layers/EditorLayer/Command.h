#pragma once
class Command {
public:
	virtual ~Command() = default;

	// Wykonuje akcje (przy robieniu i przy redo)
	virtual void Execute() = 0;


	// Cofa akcje
	virtual void Undo() = 0;
};