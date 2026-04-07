#pragma once

class Timestep
{
public:

	Timestep(float time=0.0f)
		: m_Time(time) {}

	//przeciazenie operatora dzieki czemu nie musimy pisac ts.GetSeconds * speed - kiedy kompilator widzi ze mnozymy obiekt Timestep przez float sam wywoluje te funkcje i wyciaga m_Time
	operator float() const { return m_Time; }

	//gettery
	float GetSeconds() const { return m_Time; }
	float GetMilliSeconds() const { return m_Time * 1000.0f; }


private:
	//zmienna przechowywujaca czas
	float m_Time;
};