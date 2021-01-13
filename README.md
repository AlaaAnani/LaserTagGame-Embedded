# LaserTag Embedded Systems Game

Members:
* Abdulrahman Diaa 900162359
* Alaa Anani 900163247
* Ismail Shaheen 900171417

***

# Project Idea and Flow

This project aims to implement a lasertag game system on the STML432KC nucleo-board. The game logic follows a usual lasertag game. The game starts with two teams, each having up to 4 players with 100% health and 20 ammo. Whenever a player presses the trigger, an IR Frame is blasted and the ammunition is reduced. The IR Frame carries the player's identifier and the amount of damage it deals upon hitting another player (default damage = 25%). A player in the way of that IR blast would receive it and identify the shooter and the damage, reducing the health indicator and announcing the hit to the game master. This announcement is communicated through a Bluetooth frame that has the shot player's identifier, the shooter's identifier, the damage dealt and whether this bullet was the finishing blow (i.e: killed this player). The game master receives these updates, updates the Kill-Death-Ratio and game statistics and then updates the game display.

Upon death, one can go to their base camp and "revive" themselves by pushing the revive button on the base box (Revive IR Station). This box also has a "reload" button that refills the ammunition of the player (Ammo Reload IR Station). The game ends when these boxes are disabled and all the players in the game have 0% health or all 0 ammo.

# Hardware

## For Game Master
* STML432KC Board
* Bluetooth Module HC-05 (ZS-040) [per player or replace with a WiFi router and a WiFi module]
* USB-to-TTL Module

## Per Player
* STML432KC Board
* Bluetooth Module HC-05 (ZS-040) [or replace with Wifi Module]
* IR Emitter Diode
* 150-ohm resistor
* IR Receiver Module [1838b](http://wiki.sunfounder.cc/index.php?title=IR_Receiver_Module)
* 16x2 LCD Module with daughter board (I2C)
* [OPTIONAL]: Lens with enclosure to focus emitter diode rays to a laser beam [Details Here](https://openlasertag.org/language/en/optical-emmiter-tube/)

# Software
* µVision IDE - Keil
* STM Studio
* Tera Term

# System Architecture
The architecture for this project is illustrated in the diagram below with two main systems: 
* Game Master System (The Judge)
* Player System (The Gun)

![overall_archi](https://user-images.githubusercontent.com/30525304/102684616-cc3e9580-41e2-11eb-82a8-ddc3ddd39565.png)

*Game Architecture*

# Player System
The player system models the functionality of the gun which has the IR blaster, a trigger to control it, the IR receiver (for detecting when shot), an LCD display to show current resources (health and ammo) and a Bluetooth module to communicate with the Game master. The architecture is illustrated below.
## Architecture

![player_system](https://user-images.githubusercontent.com/30525304/102684617-ce085900-41e2-11eb-9f6f-61ac2183e003.png)

*Player System Architecture*

## Circuit

![Player front](https://user-images.githubusercontent.com/28742206/102685250-0fe7ce00-41e8-11eb-9f66-d5f00b2de808.jpg)

*Player Circuit Front*
![Player back](https://user-images.githubusercontent.com/28742206/102685251-1118fb00-41e8-11eb-8824-ce6542910b35.jpg)

*Player Circuit Back*

![Player only shooting circuit](https://user-images.githubusercontent.com/28742206/102685252-11b19180-41e8-11eb-8780-ec04197d2427.jpg)

*Player Circuit (Shooting functionality only)*

## Trigger

The trigger functionality is implemented on a push button that is connected to a GPIO pin on the nucleo board. The GPIO pin is configured with a pull down resistor. The signal is being polled every iteration in the main loop and debounced to remove the noise and flickering in the button. Whenever the debounced signal is detected as high (button is pushed), the shooting mechanism is executed only once, releasing one IR bullet frame.

Debouncing:
```
static inline void debounce(void)
{
	// Counter for number of equal states
	static uint8_t count = 0;
	// Keeps track of current (debounced) state
	static uint8_t button_state = 0;
	// Check if button is high or low for the moment
	uint8_t current_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
	if (current_state != button_state) 
	{
		// Button state is about to be changed, increase counter
		count++;
		if (count >= 5) {
			// The button have not bounced for some checks, change state
			button_state = current_state;
			if (current_state != 0) {
				shoot=1;
			}
			count = 0;
		}
	} else {
	// Reset counter
		count = 0;
	}
}
``` 

## Infrared Communication
To achieve the shooting functionality, the gun should be able to send IR frames that simulate the bullets in the air. These IR "bullets" contain 4 bytes:
* Shooter's Team Number
* Shooter's Player Number (Within that team)
* Bullet Damage
* Validation Byte (Sticky 0xFF): 
  * to be able to distinguish this byte from the "Heal" and "Reload" IR frames
  * to discard any random IR frames that get detected from other sources

![IR frame](https://user-images.githubusercontent.com/30525304/102684660-5850bd00-41e3-11eb-8e93-206e49a53009.png)

*IR Frame*

To facilitate this sending, we should establish a communication medium and protocol, through which, our IR blasters and receivers can communicate with such frames. As we searched in the literature, we found a lot of different protocols and tutorials that explain the process. However, we were not successful in configuring most of them for our use-case. In this section, we will describe the process we went through and the final working solution for the communication problem.

### Failed Attempts
In this subsection, we describe the failed trials towards obtaining a functional communication protocol

#### [X-Cube IRRemote Example](https://www.st.com/en/embedded-software/x-cube-irremote.html)
As per the documentation on the website, 
> The purpose of this example (X-CUBE-IRREMOTE) is to provide a generic solution for implementing an IR transmitter (a remote control device) and receiver using the timers available on the microcontrollers of the STM32F0 Series, STM32F3 Series, and STM32G0 Series. The example implements RC5 and SIRC protocols on selected evaluation boards. Implementation is designed to be easily reconfigurable for other STM32 microcontroller series, boards and protocols.

It utilizes the IR_TIM and IR_LED functionalities in selected boards (supposedly also including STM32L432KC) for sending IR signals. However, these functionalities were associated with pin PA13 which was not exposed in the evaluation board as it's used for serial debugging. This was the main reason why we could not use this library. However, it had many problems including but not limited to: 
* Not easily configurable for STM32L432KC (have to change a lot of parameters, defines, and many peripherals in multiple areas inside the code)
 * Code was not parametrized in a clean way that allows porting it to another board.
* The IR_TIM and IR_LED functionalities are associated with PA13 which is not exposed (used for serial debugging)
* Implements a very specific version of RC5 and SIRC (not flexible or tolerant to noise)
* Utilizes up to 3 timers (2 of them have to be specific ones [15 and 16 in this case], without justifying this limitation, or providing anyway to change it)
* The documentation was not consistent with the example implementation in the syntax and so it took a lot of time debugging

However, the [documentation](https://www.st.com/resource/en/application_note/dm00271524-implementation-of-transmitters-and-receivers-for-infrared-remote-control-protocols-with-stm32cube-stmicroelectronics.pdf) for this example was very rich with information about the IR communication primitives themselves including but not limited to:

* Manchester Encoding
* RC5 encoding and decoding flowcharts
* SIRC encoding and decoding flowcharts

So, while this example provided us with valuable theoretical information about the concepts, it cost us a lot of time to try and get it running and it was never successful. Even when we removed the sending functionality and tried only the receiving, it would only work with some buttons of some remote (and not consistently; so it would produce different outputs every time). This goes back to the very specific implementation of the protocols that does not tolerate slight changes between different remotes and so this was completely discarded and we started to look for a more general solution.

#### [IR Forwarder Module](https://www.electrodragon.com/product/ir-infrared-signal-forwarder-decoder-edir/)

This module promises to act independently on "99% devices", providing a UART interface on which you can send and receive IR frames transparently. However, the modules we were able to get our hands on from the workshop were faulty; they simply did not respond at all; nor sending or receiving through UART or IR, so these were easily discarded within a day.

### STMStudio

STM Studio is a non-intrusive tool that provides real-time monitoring and visualizations of variables for STM32 microcontrollers. We found this software as we were trying to find ways to identify the protocols implemented in the remote controls we have. This way, we could implement the receiving part of this protocol specifically, debug assuming the correctness of the remote itself and have a model to compare against when implementing the sending functionality ourselves. We were able to find a more general solution to the IR communication problem itself before utilizing this tool to the fullest. However, we think it's very valuable and useful to be able to monitor and graph the variables in real-time like that. Therefore, we recorded a demo of how to use the software for anyone who might need to see it:

[![STM Studio Video](https://drive.google.com/uc?export=view&id=17kuWmw0WJmhBd2l13ZBmz_8FjHaWeMc2)](https://drive.google.com/file/d/11mFFB7U65oA-eivFRCFpv7TWSkVBmTVn/view?usp=sharing)

### [Arduino IRremote library (The Successful Attempt)](https://github.com/Arduino-IRremote/Arduino-IRremote)
This is an Arduino library that implements 40 IR protocols that are similar to RC5; they use the same Manchester encoding standard. This library is also capable of detecting which protocol was transmitted, the number of bits used, and the command that was sent. It's written in C++, so it is easy to convert the required primitives to make it work on our nucleo board, without relying on Arduino software/hardware. The implementation works on one timer only that switches between sending and receiving modes; a desired behavior for our use-case since we don't want the player's own IR blasts to hit him/her. 

For this, we found [STM32-IR Blogpost](https://sudonull.com/post/26526-IR-remote-control-on-stm32) which basically describes a project that converts the library's functionality to HAL driver code. This example [(STM32-IR Repo)](https://github.com/stDstm/Example_STM32F103/tree/master/IR_rec_trans) just requires a 72MHz timer with PWM generation for the IR blaster and an input pin for the IR receiver. We created a CubeMX project with a 72 MHz timer TIM2 with PWM generation on CH1 and an input GPIO pin (PA0) for the receiving signal and ported the repository code in the project and, with some tweaks and debugging, it finally worked! The initial receiving and transmitting repository for STML432KC can be found [here](https://aucegypt0-my.sharepoint.com/:f:/g/personal/a_diaa_aucegypt_edu/EgIDD7bB_jpIpzfP2z1G880B2QNnUzl23dkq-mn7dPOjBQ?e=vNpenl). This [folder](https://drive.google.com/drive/folders/13L0vntv-Bqk--_ghjn3F9kbrYDT2uK9X?usp=sharing) has 2 demos showcasing the functionality of this repository.

After some experimentation with different protocols and remotes, we found the SAMSUNG protocol to be the most reliable and flexible. Setting the number of bits to 32 would allow us to send the 4 bytes we previously described in our IR frame. Additionally, we could use a SAMSUNG remote to emulate the base station; all of the remote's buttons do not have the sticky 0xFF byte and so any 2 buttons would be able to provide the "revive" and "reload" functionalities. For that, we selected the "Channel Up" button for revive and "Volume Up" for reload. Therefore, in our final repository, we only include the implementation of the SAMSUNG protocol, but the rest of the protocols can be found in the [example](https://aucegypt0-my.sharepoint.com/:f:/g/personal/a_diaa_aucegypt_edu/EgIDD7bB_jpIpzfP2z1G880B2QNnUzl23dkq-mn7dPOjBQ?e=vNpenl).
 

### LCD Display
The main HMI for our system is a 16x2 LCD that displays information about the current player stats, ie. health and ammo. Normally to interact with the LCD you would do so through the 8-bit/4-bit bus to transmit the data to be displayed. However, we only had access to an LCD with an I2C daughter board that enables sending data over the I2C bus, which is more convenient for software development and provides an extension for the available GPIO pins, but adds on the overall per-unit cost. For actual production, direct interaction with the LCD is preferred as there is no critical need for the extra GPIO pins (coming from the fact that we are using only 2 pins for I2C instead of 4 for the LCD).

![KS0066U block diagram](https://i.imgur.com/SsQiASo.png  "KS0066U block diagram")

Regardless of the way we communicate with the LCD, its driver has 2 registers that we interact with:
- Data Register (DR)
- Instruction Register (IR)

The Data register is a temporary storage to store data during read or write operations. The characters that need to be displayed on the LCD are placed by the microcontroller on this register. The location on the screen is determined by issuing an instruction to the Instruction register. For a complete list of the available instructions check the datasheet of the KS0066U LCD driver in the references. It worth noting that read, write, and issuing instruction operations are determined by addressing the RS, R/W pins.

#### I2C

![PCF8574 block diagram](https://i.imgur.com/7Gf93Hy.png "PCF8574 block diagram")

As said before, instead of directly addressing the I/O buffer of the LCD, we can send both data and instructions over the I2C bus using a daughter board. The PCF8574/74A found on our LCD is a Remote 8-bit I/O expander for I2C-buses. It consist of eight quasi-bidirectional ports, 100 kHz I2C-bus interface, three hardware address inputs and interrupt output operating between 2.5 V and 6 V. Internally the PCF8574 is a shift register that will be used as the connecting medium between the I2C and the I/O buffer. The quasi-bidirectional ports are connected to the LCD I/O buffer and we interface through its I2C. The three hardware addresses are hard-wired to a logic one in our board and they represent the lower [1:4] bits of the address. The higher [4:8] bits are 0x4 and the first bit [0] is the R/W signal. Thus the I2C address that we will use to interact with the LCD is 0x4E.

#### ASCII Art

![CGROM table](https://i.imgur.com/ZFKSZEt.jpg "CGROM table")

The data we send to the DR in the LCD is not directly shown on the display, instead, it is mapped to a set of predefined characters in Character Generation ROM. The location of every character is its ASCII value. That's why when we send the character it gets displayed as we are actually specifying its address. Fortunately, we can also define up to 8 special characters and access them through the reserved first 8 locations in the CGROM. To do this, we need to write values to the CGRAM area specifying which pixels to glow.

![5x8 dot matrix](https://i.imgur.com/W48NrRy.png "5x8 dot matrix")

The LCD uses a 5x8 dot matrix to represent a character. An array of 8 bytes where the lower 5 bits of each one decide whether the pixel will glow or not. A value of 1 means the pixel is on and a 0 means it is off. The following code snippet shows the process but without specifying the sending logic as it depends on the used interface.
``` 'C'
unsigned char Pattern1 [ ] = {
    0b00000,
    0b01010,
    0b10101,
    0b01010,
    0b00100,
    0b00000,
    0b00000,
    0b00000
} ;
void CreateCustomCharacter (unsigned char *Pattern, const char Location)
{ 
    int i = 0;
    // Send the Address of CGRAM (0x40) to write the pattern to
    lcd_cmd(0x40 + (Location * 8));
    for (i = 0; i < 8; i++)
    // Send the pattern as data bytes to be stored to the CGRAM 
    lcd_data(Pattern[i]);               
}
```
The pattern here is the one we used to display the heart shape that represents the health bar

## Bluetooth Communication
Upon receiving a valid IR bullet (IR frame with a sticky 0xFF byte), the nucleo board in the player system transmits through UART to the HC-05 slave the Bluetooth frame to be transmitted to the master. The frame and how it is handled will be described in detail in the Game Master's section.

# Game Master
## Architecture

The game Master is composed of the following:

1. Bluetooth module HC-05 (ZS-040) to receive the data from the players to keep track of the Kills, Deaths and Damage. 
2. An STM32 microcontroller (L432KC) that receives the readings from the HC-05 and does the score-keeping the logic.
3. USB-to-TTL to serially receive the scores from the controller and show it on Tera Term’s terminal.

![master_archi](https://user-images.githubusercontent.com/30525304/102684901-204a7980-41e5-11eb-8542-659726b4801e.png)

*Game Master Architecture*

## Circuit 
![Master](https://user-images.githubusercontent.com/28742206/102685253-124a2800-41e8-11eb-9e8f-243d99979f57.jpg)

*Game Master Circuit*

## Bluetooth Communication
The communication used in this game system is done through Bluetooth. This is the exact module we are using:

![BT Module](https://user-images.githubusercontent.com/30525304/102684945-9058ff80-41e5-11eb-8b85-b89f55e69ee0.jpg)

*HC-05 Bluetooth Module*
(For scalability, Bluetooth can be exchanged with Wifi instead, by making a single LAN all players are connected to at the Master's side.)

Every player has an HC-05 configured as a slave by setting its role to 0, and it automatically connects to its corresponding Master HC-05 at the Master side. To configure the Bluetooth modules to enable such a communication, we did the following:

1. **Connections**: We connect the BT module in an AT command mode to a USB-to-TTL module:

* BT EN – USB-to-TTL 5V
* BT VCC— USB-to-TTL 5V
* BT GND – USB-to-TTL GND
* BT TX – USB-to-TTL TX
* BT RX – USB-to-TTL RX

2. **Slave Configuration**:
We send through Tera Term's monitor the following commands (setting the baudrate to 38400, which is the baudrate the HC-05 operates with in AT mode)
```
AT //if the module sends “OK” back, then it is correctly communicating
AT+ROLE=0 //sets the module in slave mode
AT+ADDR? //returns the address of the slave
```
And we save the slave’s address to make the master bind to it later.

3. **Master Configuration**:
```
AT //if the module sends “OK” back, then it is correctly communicating
AT+ROLE=1 //sets the module in master mode
AT+BIND=98d3,71,fd7d72 //assuming the slave’s address is 98d3:71:fd7d72
```
4. Now if we power both master and salve modules, they will take seconds to pair with each other automatically. Once this happens, their LEDs will blink every 2 seconds.

5. Whatever we serially write to the slave will be transmitted wirelessly to the master and the master will automatically output what it received serially as well to the nucleo board at its side.

To keep track of the Deaths, Kills, Damage and KDR of every player in the game, whenever any player gets shot from another player, their BT modules transmits to the master’s BT module the following 6-byte packet, where every field is a single byte:
![BT frame](https://user-images.githubusercontent.com/30525304/102685010-0493a300-41e6-11eb-9f67-ee38c97e8f87.png)

## Scoreboard

The master updates the scores based on that values based on the received Bluetooth frames from the players. It prints the scores serially through UART2 to the Tera Term terminal by clearing the terminal window automatically and writing the new scores to update in-place.
Here is a gif of the terminal while Player 1 from Team 2 is shooting (and eventually killing) Player 1 from Team 1:

![MasterTerminal](https://user-images.githubusercontent.com/30525304/102685165-2d686800-41e7-11eb-9377-49dfe001a82c.gif)

*Master Terminal*

# Demo

[![demo picture](https://user-images.githubusercontent.com/28742206/102685406-027f1380-41e9-11eb-938b-3d832af4685b.jpg)](https://drive.google.com/file/d/1WjAgUhbl73kTtCg4Q-vlt9X-Z50Y2-LA/view?usp=sharing)

*Demo*

# Useful Links
* [Arduino IRremote library](https://github.com/Arduino-IRremote/Arduino-IRremote)
* [STM32-IR Blogpost](https://sudonull.com/post/26526-IR-remote-control-on-stm32)
* [STM32-IR Repo](https://github.com/stDstm/Example_STM32F103/tree/master/IR_rec_trans)
* [X-Cube IRRemote Example](https://www.st.com/en/embedded-software/x-cube-irremote.html)
* [16COM / 40SEG DRIVER & CONTROLLER FOR DOT MATRIX LCD](https://reference.digilentinc.com/_media/reference/pmod/pmodclp/ks0066.pdf)
* [PCF8574; PCF8574A: Remote 8-bit I/O expander for I2C-bus with interrupt](https://www.nxp.com/docs/en/data-sheet/PCF8574_PCF8574A.pdf)
* [How To Configure and Pair Two HC-05 Bluetooth Modules as Master and Slave](https://howtomechatronics.com/tutorials/arduino/how-to-configure-pair-two-hc-05-bluetooth-module-master-slave-commands/)
