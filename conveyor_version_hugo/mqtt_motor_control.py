# This code is a simple MQTT client that allows you to control a motor (or conveyor) by sending messages to a specific topic.
# The code uses the paho-mqtt library to connect to a MQTT broker and publish messages to a topic called "motor/control". 
# The user can choose to turn the motor on (send "1") or off (send "0") by entering the corresponding option in the console.

import paho.mqtt.client as mqtt

# MQTT Configurations
broker = "localhost"  # Same as "127.0.0.1" or use the IP address of your broker
port = 1883        
topic = "motor/control"  # Topic to control the motor

def send_message(state):
    """
    Envia uma mensagem MQTT com o estado do motor (0 ou 1).
    :param state: 0 para desligar o motor, 1 para ligar o motor
    """
    client = mqtt.Client()  # Cria um cliente MQTT
    try:
        # Conecta ao broker MQTT
        client.connect(broker, port)
        print(f"Connected to MQTT broker at {broker}:{port}")

        # Publica a mensagem no tópico
        client.publish(topic, str(state))
        print(f"Message sent: {state} to topic '{topic}'")

    except Exception as e:
        print(f"Error sending the message: {e}")

    finally:
        client.disconnect()

if __name__ == "__main__":
    while True:
        print("\nControl of the motor:")
        print("1 - Turn ON the motor")
        print("0 - Turn OFF the motor")
        print("q - Quit")
        
        choice = input("Choose an option: ").strip()
        # These values were chosen to be 1 and 0 because in the code of the PLC we check if the value received is 1 or 0 to turn on or off the motor, thus you can change these values in both codes if you want to use other values for controlling the motor.
        if choice == "1":
            send_message(1)  # Sends 1 to turn on the motor
        elif choice == "0":
            send_message(0)  # Sends 0 to turn off the motor
        elif choice.lower() == "q":
            print("Quitting...")
            break
        else:
            print("Invalid option! Please choose 1, 0 or q.")