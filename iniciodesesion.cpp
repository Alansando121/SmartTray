#include <iostream>
#include <string>
//holaaaaa putasss
using namespace std;

int main() {
    const string usuarioCorrecto = "admin";
    const string passwordCorrecto = "1234";

    string usuario;
    string password;
    int intentos = 3;

    cout << "====================================\n";
    cout << "      SmartTray - Inicio de sesion\n";
    cout << "====================================\n";

    while (intentos > 0) {
        cout << "Usuario: ";
        cin >> usuario;

        cout << "Contraseña: ";
        cin >> password;

        if (usuario == usuarioCorrecto && password == passwordCorrecto) {
            cout << "\nAcceso concedido. Bienvenido, " << usuario << "!\n";
            return 0;
        }

        intentos--;

        if (intentos > 0) {
            cout << "\nCredenciales incorrectas. Te quedan " << intentos
                 << " intento(s).\n\n";
        } else {
            cout << "\nCredenciales incorrectas. No te quedan mas intentos.\n";
            cout << "Acceso denegado.\n";
        }
    }

    return 1;
}
