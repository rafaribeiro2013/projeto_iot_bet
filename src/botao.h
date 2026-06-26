#pragma once

#include <Arduino.h>
#include <GFButton.h>
#include "sessao.h"

void botaoInit();
void botaoLoop();
void botaoUpPressionado(GFButton& botaoDoEvento);
void botaoCenterPressionado(GFButton& botaoDoEvento);
void botaoDownPressionado(GFButton& botaoDoEvento);