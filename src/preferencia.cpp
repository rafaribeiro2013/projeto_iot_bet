#include "preferencia.h"

void initPreferences() {
  preferencias.begin("numeroMesa");
  preferencias.putInt("mesa", 3); 
}

int getNumeroMesa() {
    return preferencias.getInt("mesa", 0);
}