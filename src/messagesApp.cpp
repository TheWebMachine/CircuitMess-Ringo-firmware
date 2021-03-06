#include "messagesApp.h"
#include "contactsApp.h"
int16_t y = 0;
/*
[
	{
		number: "123123",
		dateTime: "2018-12-12 12:12:12",
		text: "asd asd asd asd"
	}
]
*/

//Messages app
void messagesApp() 
{
	Serial.println("Load messages app");
	int16_t menuChoice = -1;
	mp.dataRefreshFlag = 0;
	File file = SD.open("/.core/messages.json", "r");

	if(file.size() < 2){ // empty -> FILL
		Serial.println("Override");
		file.close();
		jb.clear();
		// JsonArray& jarr = jb.parseArray("[{\"number\":\"123123\", \"dateTime\":\"2018-12-12 12:12:12\", \"text\":\"asd asd asd asd\"}]");
		// JsonArray& jarr = jb.parseArray("[{\"number\":\"123123\", \"dateTime\":\"2018-12-12 12:12:12\", \"text\":\"asd asd asd asd\"}, {\"number\":\"09123\", \"dateTime\":\"2018-12-12 12:12:12\", \"text\":\"Some other text\"}, {\"number\":\"911\", \"dateTime\":\"2018-03-12 12:12:12\", \"text\":\"Help\"}]");
		JsonArray& jarr = jb.createArray();
		delay(10);
		File file1 = SD.open("/.core/messages.json", "w");
		jarr.prettyPrintTo(file1);
		file1.close();
		file = SD.open("/.core/messages.json", "r");
		if(!file.available())
			Serial.println("Messages ERROR");
		Serial.println(file.size());
	}

	jb.clear();
	JsonArray& jarr = jb.parseArray(file);
	file.close();
	jarr.prettyPrintTo(Serial);
	if(!jarr.success())
	{
		Serial.println("Error");
		mp.display.fillScreen(TFT_BLACK);
		mp.display.setCursor(0, mp.display.height()/2 - 16);
		mp.display.setTextFont(2);
		mp.display.printCenter("Error loading data");
		jb.clear();
		while (mp.buttons.released(BTN_B) == 0)//BUTTON BACK
			mp.update();
		while(!mp.update());
	}
	else
	{
		while (1)
		{
			menuChoice = smsMenu(jarr, menuChoice);
			Serial.print("chosen: ");
			Serial.println(menuChoice);

			if (menuChoice == -1)
				break;

			else if (menuChoice == -2)
			{
				if(mp.sim_module_version == 255)
				{
					mp.display.fillScreen(TFT_BLACK);
					mp.display.setTextColor(TFT_WHITE);
					mp.display.setTextSize(1);
					mp.display.setCursor(0, mp.display.height()/2 - 20);
					mp.display.setTextFont(2);
					mp.display.printCenter(F("No network board!"));
					mp.display.setCursor(0, mp.display.height()/2);
					mp.display.printCenter(F("Insert board and reset"));
					uint32_t tempMillis = millis();
					while(millis() < tempMillis + 2000 && !mp.buttons.released(BTN_A) && !mp.buttons.released(BTN_B))
						mp.update();
					while(!mp.update());
				}
				else if(!mp.simInserted)
				{
					mp.display.fillScreen(TFT_BLACK);
					mp.display.setCursor(0, mp.display.height()/2 - 20);
					mp.display.setTextFont(2);
					mp.display.printCenter(F("No SIM inserted!"));
					mp.display.setCursor(0, mp.display.height()/2);
					mp.display.printCenter(F("Insert SIM and reset"));
					uint32_t tempMillis = millis();
					while(millis() < tempMillis + 2000 && !mp.buttons.released(BTN_A) && !mp.buttons.released(BTN_B))
						mp.update();
					while(!mp.update());
				}
				else if(mp.airplaneMode)
				{
					mp.display.fillScreen(TFT_BLACK);
					mp.display.setCursor(0, mp.display.height()/2 - 20);
					mp.display.setTextFont(2);
					mp.display.printCenter(F("Can't send SMS!"));
					mp.display.setCursor(0, mp.display.height()/2);
					mp.display.printCenter(F("Turn off airplane mode"));
					uint32_t tempMillis = millis();
					while(millis() < tempMillis + 2000 && !mp.buttons.released(BTN_A) && !mp.buttons.released(BTN_B))
						mp.update();
					while(!mp.update());
				}
				else
				{
					bool readyForCall = 0;
					uint32_t timeoutMillis = millis(); 
					while(Serial1.available())
						Serial1.read();
					while(!readyForCall && millis() - timeoutMillis < 10000)
					{
						if(mp.sim_module_version == 1)
						{
							Serial1.println("AT+CCALR?");
							String input = mp.waitForOK();

							uint16_t helper = input.indexOf(" ", input.indexOf("+CCALR:"));
							readyForCall = input.substring(helper + 1, helper + 2).toInt();
							Serial.println(input);
							if(!readyForCall)
							{
								mp.display.fillScreen(TFT_BLACK);
								mp.display.setTextColor(TFT_WHITE);
								mp.display.setTextSize(1);
								mp.display.setCursor(0, mp.display.height()/2 - 20);
								mp.display.setTextFont(2);
								mp.display.printCenter(F("Registering to network"));
								mp.display.setCursor(0, mp.display.height()/2);
								mp.display.printCenter(F("Please wait..."));
								while(!mp.update());
								delay(1000);
							}
						}
						else
						{
							Serial1.println("AT+CREG?");
							String input = mp.waitForOK();

							uint16_t helper = input.indexOf(",", input.indexOf("+CREG:"));
							readyForCall = input.substring(helper + 1, helper + 2).toInt();
							Serial.println(input);
							if(!readyForCall)
							{
								mp.display.fillScreen(TFT_BLACK);
								mp.display.setTextColor(TFT_WHITE);
								mp.display.setTextSize(1);
								mp.display.setCursor(0, mp.display.height()/2 - 20);
								mp.display.setTextFont(2);
								mp.display.printCenter(F("Registering to network"));
								mp.display.setCursor(0, mp.display.height()/2);
								mp.display.printCenter(F("Please wait..."));
								while(!mp.update());
								delay(1000);
							}
						}
					}
					mp.networkRegistered = readyForCall;
					if(readyForCall)
					{
						if(mp.signalStrength == 99)
						{
							Serial1.println("AT+CSQ");
							String buffer = "";
							uint32_t current = millis();
							while(buffer.indexOf("+CSQ:") == -1 && millis() - current >= 2000)
								buffer = Serial1.readString();
							if(buffer.indexOf("+CSQ:") != -1)
								mp.signalStrength = buffer.substring(buffer.indexOf(" ", buffer.indexOf("+CSQ:")) + 1, buffer.indexOf(",", buffer.indexOf(" ", buffer.indexOf("+CSQ:")))).toInt();
							if(mp.signalStrength == 99)
							{
								mp.display.fillScreen(TFT_BLACK);
								mp.display.setTextColor(TFT_WHITE);
								mp.display.setTextSize(1);
								mp.display.setCursor(0, mp.display.height()/2 - 20);
								mp.display.setTextFont(2);
								mp.display.printCenter(F("No signal!"));
								mp.display.setCursor(0, mp.display.height()/2);
								mp.display.printCenter(F("Check your antenna"));
								uint32_t tempMillis = millis();
								while(millis() < tempMillis + 2000 && !mp.buttons.released(BTN_A) && !mp.buttons.released(BTN_B))
									mp.update();
								while(!mp.update());
							}
							else
							{
								composeSMS(&jarr);
								File file = SD.open("/.core/messages.json", "r");
								jb.clear();
								JsonArray& jarr = jb.parseArray(file);
								file.close();
								if(!jarr.success())
								{
									Serial.println("Error");
									mp.display.fillScreen(TFT_BLACK);
									mp.display.setCursor(0, mp.display.height()/2 - 16);
									mp.display.setTextFont(2);
									mp.display.printCenter("Error loading data");

									while (mp.buttons.released(BTN_B) == 0)//BUTTON BACK
										mp.update();
									while(!mp.update());
								}
								mp.newMessage = 0;
							}
						}
						else
						{
							composeSMS(&jarr);
							File file = SD.open("/.core/messages.json", "r");
							jb.clear();
							JsonArray& jarr = jb.parseArray(file);
							file.close();
							if(!jarr.success())
							{
								Serial.println("Error");
								mp.display.fillScreen(TFT_BLACK);
								mp.display.setCursor(0, mp.display.height()/2 - 16);
								mp.display.setTextFont(2);
								mp.display.printCenter("Error loading data");

								while (mp.buttons.released(BTN_B) == 0)//BUTTON BACK
									mp.update();
								while(!mp.update());
							}
							mp.newMessage = 0;
						}
					}
					else
					{
						mp.display.fillScreen(TFT_BLACK);
						mp.display.setTextColor(TFT_WHITE);
						mp.display.setTextSize(1);
						mp.display.setCursor(0, mp.display.height()/2 - 20);
						mp.display.setTextFont(2);
						mp.display.printCenter(F("Network unavailable"));
						mp.display.setCursor(0, mp.display.height()/2);
						mp.display.printCenter(F("Try again later"));
						uint32_t tempMillis = millis();
						while(millis() < tempMillis + 2000 && !mp.buttons.released(BTN_A) && !mp.buttons.released(BTN_B))
							mp.update();
						while(!mp.update());
					}
				}
				menuChoice = -1;
			}

			else if(menuChoice == -3)
			{
				File file = SD.open("/.core/messages.json", "r");
				jb.clear();
				JsonArray& jarr = jb.parseArray(file);
				file.close();
				if(!jarr.success())
				{
					Serial.println("Error");
					mp.display.fillScreen(TFT_BLACK);
					mp.display.setCursor(0, mp.display.height()/2 - 16);
					mp.display.setTextFont(2);
					mp.display.printCenter("Error loading data");

					while (mp.buttons.released(BTN_B) == 0)//BUTTON BACK
						mp.update();
					while(!mp.update());
				}
				mp.newMessage = 0;
				menuChoice = -1;
			}

			else if(menuChoice > -1)
			{
				bool helper = 0;
				String temp = jarr[menuChoice]["contact"].as<char*>();
				if(temp == "")
				{
					helper = viewSms(jarr[menuChoice]["text"].as<char*>(), jarr[menuChoice]["number"].as<char*>(),
					jarr[menuChoice]["dateTime"].as<uint32_t>(), jarr[menuChoice]["direction"].as<bool>());
				}
				else
				{
					helper = viewSms(jarr[menuChoice]["text"].as<char*>(), jarr[menuChoice]["contact"].as<char*>(),
					jarr[menuChoice]["dateTime"].as<uint32_t>(), jarr[menuChoice]["direction"].as<bool>());
				}
				if(helper)
				{
					jarr.remove(menuChoice);
					File file = SD.open("/.core/messages.json", "w");
					jarr.prettyPrintTo(file);
					file.close();
					menuChoice--;
				}
				else if(!jarr[menuChoice]["read"].as<bool>())
				{
					jarr[menuChoice]["read"] = 1;
					File temp = SD.open("/.core/messages.json", "w");
					jarr.prettyPrintTo(temp);
					temp.close();
					file = SD.open("/.core/messages.json", "r");
					if(!file.available())
						Serial.println("Messages ERROR");
					jb.parseArray(file);
					file.close();
				}
			}
			else
			{
				menuChoice = -(menuChoice + 3);
				menuChoice--;
			}
		}
	}
}

bool viewSms(String content, String contact, uint32_t date, bool direction) 
{
	String input = "";
	String buffer = "";
	uint16_t awayX = 0;
	String helpString = "";
	int16_t leftX = 0;
	int16_t oldX = 0;
	int16_t oldY = 0;
	bool lineFlag = false;
	y = 14;  //Beggining point
	unsigned long elapsedMillis = millis();
	bool blinkState = 1;
	DateTime time = DateTime(date);
	String contactLabel = mp.checkContact(contact);

	for(int i = 0; i < 10; i++)
	{
		if(mp.notificationTypeList[i] == 2 && mp.notificationDescriptionList[i] == contact && mp.notificationTimeList[i] == time)
		{
			mp.removeNotification(i);
			break;
		}
	}
	while (1)
	{
		mp.display.fillScreen(TFT_DARKGREY);
		mp.display.setTextWrap(1);
		mp.display.setCursor(2, y);
		lineFlag = false;
		for(uint16_t i = 0; i < content.length(); i++)
		{
			leftX = 154 - mp.display.getCursorX();
			if(mp.display.getCursorX() < 2)	mp.display.setCursor(2, mp.display.getCursorY()); 
			if( content[i] == ' ' || i==0) {
				helpString = "";
				for(uint8_t j = 1; j < 24; j++){
				if (j == 22) {
					lineFlag = true;
					mp.display.print(content[i]);
				}
				else if(content[i+j] != ' ' && i+j != content.length()-1) helpString += content[i+j];
				else {
					oldX = mp.display.getCursorX();
		        	oldY = mp.display.getCursorY();
					mp.display.setCursor(0, -20);
					mp.display.print(helpString);
					awayX = mp.display.getCursorX();
					mp.display.setCursor(oldX, oldY);
					//mp.display.print(content[i]);
					if(awayX > leftX) mp.display.println("");
					else mp.display.print(content[i]);
					if(mp.display.getCursorX() < 2)	mp.display.setCursor(2, mp.display.getCursorY()); 
					break;
					}
				}
			}
			else mp.display.print(content[i]);
			if (lineFlag && mp.display.getCursorX() > 140 ) {
				mp.display.print("-");
				mp.display.println();
				mp.display.setCursor(2, mp.display.getCursorY());
				lineFlag = false;
			}

			if(mp.display.getCursorY() > 120)
				break; 
		}

		if (mp.buttons.repeat(BTN_DOWN, 3)) { //BUTTON DOWN
			if (mp.display.cursor_y >= 94)
			{

				// if (!mp.buttons.released(BTN_DOWN))
				y -= 4;
				// while (!mp.buttons.released(BTN_DOWN))
				// {
				// 	if (millis() - buttonHeld > 100) {
				// 		y -= 4;
				// 	}
				// 	mp.update();
				// 	break;
				// }
			}
		}

		if (mp.buttons.repeat(BTN_UP, 3)) { //BUTTON UP
			if (y < 14)
			{
				y += 4;
			}
		}

		if (mp.buttons.released(BTN_B)) //BUTTON BACK
		{
			break;
		}

		if (millis() - elapsedMillis >= 1000) 
		{
			elapsedMillis = millis();
			blinkState = !blinkState;
		}
		if(mp.buttons.released(BTN_FUN_LEFT))
		{
			return 1;
		}

		if (blinkState == 1)
		{
			mp.display.setTextColor(TFT_WHITE);
			mp.display.fillRect(0, 0, mp.display.width(), 14, TFT_DARKGREY);
			mp.display.setTextFont(2);
			mp.display.setCursor(2,-1);
			mp.display.drawFastHLine(0, 14, mp.display.width(), TFT_WHITE);
			mp.display.print(direction ? "From: " : "To: ");
			if(contactLabel != "")
				mp.display.print(contactLabel);
			else
				mp.display.print(contact);
		}
		else
		{
			mp.display.setTextColor(TFT_WHITE);
			mp.display.fillRect(0, 0, mp.display.width(), 14, TFT_DARKGREY);
			mp.display.setTextFont(2);
			mp.display.setCursor(2,-1);
			mp.display.drawFastHLine(0, 14, mp.display.width(), TFT_WHITE);
			char buf[100];
			strncpy(buf, "DD.MM.YYYY hh:mm:ss\0", 100);
			mp.display.print(time.format(buf));
		}
		mp.display.fillRect(0,110, 160, 18, TFT_DARKGREY);
		mp.display.drawFastHLine(0,111, 160, TFT_WHITE);
		mp.display.setCursor(4, 112);
		mp.display.print("Erase");
		mp.update();
	}
	// while(!mp.update());
	mp.buttons.update();
	return 0;
}
void smsMenuDrawBox(String contact, DateTime date, String content, bool direction, bool isRead, uint8_t i, int32_t y) 
{
	uint8_t offset;
	uint8_t boxHeight;
	uint8_t composeHeight;
	int sms_day = date.day();
	int sms_month = date.month();
	mp.display.setTextSize(1);
	offset = 19;
	composeHeight=21;
	boxHeight = 30;
	mp.display.setTextFont(2);
	//String contactLabel = mp.checkContact(contact);
	String contactLabel = "";

	y += (i-1) * (boxHeight-1) + composeHeight + offset;
	if (y < 0 || y > mp.display.height()) 
	{
		return;
	}
	String monthsList[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
	mp.display.fillRect(1, y + 1, mp.display.width() - 2, boxHeight-2, isRead ? TFT_DARKGREY : 0xA534);

	if(direction)
		mp.display.drawBitmap(3, y + 6, incomingMessageIcon, TFT_BLUE, 2);
	else
		mp.display.drawBitmap(3, y + 6, outgoingMessageIcon, TFT_GREEN, 2);
	
	mp.display.setTextColor(TFT_WHITE);
	mp.display.setCursor(22, y - 1);
	
	if(contact.length() > 13 && contactLabel == "")
	{
		for(int i = 0; i < contact.length(); i++)
		{
			mp.display.print(contact[i]);
			if(mp.display.getCursorX() > 105)
			{
				mp.display.print("...");
				break;
			}
		}
	}
	else
	{
		if(contactLabel != "")
			mp.display.print(contactLabel);
		else
			mp.display.print(contact);
		
	}
	// mp.display.drawString(contact, 22, y-1);
	mp.display.drawString(content, 22, y + 13);

	mp.display.setTextFont(1);
	mp.display.setCursor(127, y+3);
	mp.display.print(monthsList[sms_month-1]);
	mp.display.setCursor(mp.display.getCursorX() + 2, mp.display.getCursorY());
	mp.display.print(sms_day);
}
void smsMenuDrawCursor(uint8_t i, int32_t y) 
{
	uint8_t offset;
	uint8_t boxHeight;
	uint8_t composeHeight;
	offset = 19;
	composeHeight=21;
	boxHeight = 30;
	mp.display.setTextSize(2);
	y += (i-1) * (boxHeight-1) + composeHeight + offset;
	mp.display.drawRect(0, y, mp.display.width(), boxHeight, TFT_RED);
}
int16_t smsMenu(JsonArray& messages, int16_t prevCursor) 
{
	int32_t cameraY = 0;
	int16_t cursor = 0;
	int32_t cameraY_actual = 0;
	uint8_t offset;
	uint8_t boxHeight;
	uint32_t blinkMillis = millis();
	bool blinkState = 0;
	int length = messages.size();
	uint8_t actualMessagesSize = messages.size();
	for(int i = 0; i < messages.size(); i++)
	{
		if(strlen(messages[i]["text"].as<char*>()) > 160)
			actualMessagesSize+=strlen(messages[i]["text"].as<char*>())/160;
	}
	uint16_t sortingArray[length];
	for(int i = 0; i < length; i++)
		sortingArray[i] = i;
	uint16_t min;
	for(int i = 0; i < length; i++) // sorting by unread
	{
		if(!messages[sortingArray[i]]["read"].as<bool>())
		{
			for(int j = 0; j < length; j++)
			{
				if(messages[sortingArray[j]]["read"].as<bool>() && i != j)
				{
					uint16_t temp = sortingArray[j];
					sortingArray[j] = sortingArray[i];
					sortingArray[i] = temp;
					break;
				}
			}
		}
		
	}
	for(int i = 0; i < length - 1; i++) //sorting all by latest
	{
		min = i;
		for(int j = i + 1; j < length ; j++)
			if(messages[sortingArray[j]]["dateTime"].as<uint32_t>() > messages[sortingArray[min]]["dateTime"].as<uint32_t>()
			&& ((!messages[sortingArray[i]]["read"].as<bool>() && !messages[sortingArray[j]]["read"].as<bool>())
			|| (messages[sortingArray[i]]["read"].as<bool>() && messages[sortingArray[j]]["read"].as<bool>())))
				min = j;
		uint16_t temp = sortingArray[min];
		sortingArray[min] = sortingArray[i];
		sortingArray[i] = temp;
	}
	for(int i = 0; i < length; i++)
		if(sortingArray[i] == prevCursor)
			cursor = i + 1;
	Serial.print("Prevcursor: ");
	Serial.println(prevCursor);
	
	// for(int i = 0; i < length; i++) // sorting unread by latest
	// {
	// 	min = i;
	// 	for(int j = i + 1; j < length ; j++)
	// 		if(messages[sortingArray[j]]["dateTime"].as<uint32_t>() > messages[sortingArray[min]]["dateTime"].as<uint32_t>()
	// 		&& !messages[sortingArray[i]]["read"].as<bool>() && !messages[sortingArray[j]]["read"].as<bool>())
	// 			min = j;
	// 	uint16_t temp = sortingArray[min];
	// 	sortingArray[min] = sortingArray[i];
	// 	sortingArray[i] = temp;
	// }
	Serial.print("Cursor: ");
	Serial.println(cursor);
	// messages.prettyPrintTo(Serial);
	// for(int i = 0; i< length; i++)
	// {
	// 	Serial.print(sortingArray[i]); Serial.print(", ");
	// }
	if(prevCursor == -1)
		cursor = 0;
	offset = 19;
	boxHeight = 30;
	if (length > 2 && cursor > 2) 
	{
		cameraY = -(cursor - 1) * (boxHeight - 1) + offset + 11 ;
	}
	while (1) 
	{
		mp.update();
		mp.display.fillScreen(TFT_BLACK);
		mp.display.setCursor(0, 0);
		cameraY_actual = (cameraY_actual + cameraY) / 2;
		if (cameraY_actual - cameraY == 1) 
		{
			cameraY_actual = cameraY;
		}

		uint8_t i = 0;

		smsMenuComposeBox(i, cameraY_actual);
		for(int i = 0; i < length; i++)
		{
			JsonObject& elem = messages[sortingArray[i]];
			DateTime date = DateTime(elem["dateTime"].as<uint32_t>());
			String temp = elem["contact"].as<char*>();
			if(temp == "")
				smsMenuDrawBox(elem["number"].as<char*>(), date, elem["text"].as<char*>(), elem["direction"].as<bool>(),
				elem["read"].as<bool>(), i+1, cameraY_actual);
			else
				smsMenuDrawBox(elem["contact"].as<char*>(), date, elem["text"].as<char*>(), elem["direction"].as<bool>(),
				elem["read"].as<bool>(), i+1, cameraY_actual);
		}
		if(millis() - blinkMillis >= 300)
		{
			blinkMillis = millis();
			blinkState = !blinkState;
		}
		if(blinkState)
		{
			if (cursor == 0)
				smsMenuComposeBoxCursor(cursor, cameraY_actual);
			else
				smsMenuDrawCursor(cursor, cameraY_actual);
		}

		mp.display.fillRect(0, 0, mp.display.width(), 14, TFT_DARKGREY);
		mp.display.setTextFont(2);
		mp.display.setCursor(1,-1);
		mp.display.drawFastHLine(0, 14, BUF2WIDTH, TFT_WHITE);
		mp.display.setTextSize(1);
		mp.display.setTextColor(TFT_WHITE);
		mp.display.print("Messages");
		mp.display.print("          ");
		mp.display.printf("%d/%d", actualMessagesSize, SMS_LIMIT);
		mp.display.drawFastHLine(0, 112, BUF2WIDTH, TFT_WHITE);
		mp.display.fillRect(0, 113, mp.display.width(), 30, TFT_DARKGREY);
		mp.display.setCursor(5, 113);
		mp.display.printCenter("Erase               View");
		mp.display.setTextSize(1);
		mp.display.setTextColor(TFT_WHITE);

		if(mp.buttons.released(BTN_HOME)) {
			mp.exitedLockscreen = true;
			mp.lockscreen();
		}

		if (mp.buttons.released(BTN_A) || mp.buttons.released(BTN_FUN_RIGHT)) 
		{   //BUTTON CONFIRM
			mp.osc->note(75, 0.05);
			mp.osc->play();
			while(!mp.update());// Exit when pressed
			break;
		}

		if (mp.buttons.released(BTN_UP)) 
		{  //BUTTON UP
			blinkMillis = millis();
			blinkState = 1;
			mp.osc->note(75, 0.05);
			mp.osc->play();

			if (cursor == 0) 
			{
				cursor = length;
				if (length > 2) 
				{
					cameraY = -(cursor - 1) * (boxHeight-1);
				}
			}
			else 
			{
				cursor--;
				if (cursor > 0 && ((cursor-1) * (boxHeight-1) + cameraY + offset + 21) < 20) 
				{
					cameraY += (boxHeight-1);
				}
			}
		}

		if (mp.buttons.released(BTN_DOWN)) 
		{ //BUTTON DOWN
			blinkMillis = millis();
			blinkState = 1;
			mp.osc->note(75, 0.05);
			mp.osc->play();

			cursor++;
			if (cursor > 0)
			{
				if (((cursor - 1) * (boxHeight-1) + composeBoxHeight + cameraY + offset + 21) > 100) 
				{
					cameraY -= boxHeight-1;
				}
			}
			else
			{
				if ((cursor * (boxHeight-1) + cameraY + offset + 21) > 100) 
				{
					cameraY -= boxHeight-1;
				}
			}
			if (cursor > length) 
			{
				cursor = 0;
				cameraY = 0;

			}

		}

		if (mp.buttons.released(BTN_B)) //BUTTON BACK
			return -1;
		
		if(mp.buttons.released(BTN_FUN_LEFT) && cursor > 0)
		{
			String temp = messages[sortingArray[cursor - 1]]["contact"].as<String>();
			for(int i = 0; i < 10; i ++)
			{
				if(temp == "")
				{
					if(mp.notificationTypeList[i] == 2 &&
					mp.notificationDescriptionList[i] == messages[sortingArray[cursor - 1]]["number"].as<String>() &&
					mp.notificationTimeList[i] == DateTime(messages[sortingArray[cursor - 1]]["dateTime"].as<uint32_t>()))
					{
						mp.removeNotification(i);
						break;
					}
				}
				else
				{
					if(mp.notificationTypeList[i] == 2 &&
					mp.notificationDescriptionList[i] == messages[sortingArray[cursor - 1]]["contact"].as<String>() &&
					mp.notificationTimeList[i] == DateTime(messages[sortingArray[cursor - 1]]["dateTime"].as<uint32_t>()))
					{
						mp.removeNotification(i);
						break;
					}
				}
			}
			messages.remove(sortingArray[cursor - 1]);
			File file = SD.open("/.core/messages.json", "w");
			messages.prettyPrintTo(file);
			file.close();
			return -3 - sortingArray[cursor-1];

		}
		if(mp.newMessage)
		{
			mp.newMessage = 0;
			return -3;
		}
	}
	if(cursor > 0)
	{
		cursor = sortingArray[cursor - 1];
		return cursor;
	}
	else
		return -2;
}
void smsMenuComposeBoxCursor(uint8_t i, int32_t y) 
{
	uint8_t offset;
	uint8_t boxHeight;
	offset = 19;
	boxHeight=21;
	y += offset;
	mp.display.drawRect(0, y, mp.display.width(), boxHeight+1, TFT_RED);
}
void smsMenuComposeBox(uint8_t i, int32_t y) 
{
	uint8_t scale;
	uint8_t offset;
	uint8_t boxHeight;
	mp.display.setTextSize(1);
    scale = 2;
    offset = 19;
    boxHeight=21;
    mp.display.setTextFont(2);
	y += offset;
	if (y < 0 || y > mp.display.height()) 
	{
		return;
	}
	mp.display.fillRect(1, y + 1, mp.display.width() - 2, boxHeight-1, TFT_DARKGREY);
	mp.display.drawBitmap(2*scale, y + 2, composeIcon, TFT_WHITE, scale);
	mp.display.setTextColor(TFT_WHITE);
	mp.display.drawString("New SMS", 20*scale, y+3);
	mp.display.setFreeFont(TT1);

}
void composeSMS(JsonArray *messages)
{
	mp.textInput("");
	mp.textPointer = 0;
    y = 16; //beginning point
	String content = "";
	String contact = "+";
	String prevContent = "";
	int contactID = 0;
	String contactTemp = "";
	char key = NO_KEY;
	bool cursor = 0; //editing contacts or text content
	unsigned long elapsedMillis = millis();
	bool blinkState = 1;
	uint8_t scale = 2;
	bool plusSign = 0;
	bool helpPop;
	bool contactsApp = false;
	String contactLabel = "";
	bool contactsEmpty = 0;
	File file = SD.open("/.core/contacts.json", "r");
	if (file.size() < 2)
	{
		contactsEmpty = 1;
		Serial.println("Override");
		file.close();
		jb.clear();
		JsonArray &jarr = jb.createArray();
		delay(10);
		File file1 = SD.open("/.core/contacts.json", "w");
		jarr.prettyPrintTo(file1);
		file1.close();
		file = SD.open("/.core/contacts.json", "r");
		if (!file)
			Serial.println("CONTACTS ERROR");
	}
	jb.clear();
	JsonArray &jarr = jb.parseArray(file);
	file.close();
	if(jarr.measureLength() < 3)
		contactsEmpty = 1;
	while (1)
	{
		if(mp.buttons.released(BTN_HOME)) {
			mp.exitedLockscreen = true;
			mp.lockscreen();
		}
		mp.display.fillScreen(TFT_DARKGREY);

		if (millis() - elapsedMillis >= multi_tap_threshold) //cursor blinking routine
		{
			elapsedMillis = millis();
			blinkState = !blinkState;
		}
		
		if(cursor == 1) //inputting the text content
		{
			mp.display.setTextColor(TFT_WHITE);
            mp.display.setCursor(2, -1);
			mp.display.print("To: ");
			prevContent = content;
			content = mp.textInput(content, 160);
			if (prevContent != content)
			{
				blinkState = 1;
				elapsedMillis = millis();
			}
			mp.display.setTextWrap(1);
			mp.display.setCursor(1*scale, y);
			for(int i = 0; i < content.length();i++)
			{
				mp.display.print(content[i]);
				if(mp.display.getCursorX() > 150)
					mp.display.print("\n");
			}
			mp.display.fillRect(0, 0, mp.display.width(), 14, TFT_DARKGREY);


			if(mp.display.getCursorY() > 120)
				y -= mp.display.getCursorY() - 120 + 8;
			if(y < 16 && mp.display.getCursorY() < 100)
				y += 120 - mp.display.getCursorY() + 8;
			if(blinkState == 1)	
                mp.display.drawFastVLine(mp.display.getCursorX(), mp.display.getCursorY()+3, 10, TFT_WHITE);

		}
	
		if (cursor == 0) //inputting the contact number
		{
			key = mp.buttons.getKey();
			if(mp.buttons.held(BTN_0, 20))
			{
				contact+="+";
				plusSign = 1;
			}
			if(key == '0' && plusSign)
				key = NO_KEY;
			if(plusSign && mp.buttons.released(BTN_0))
				plusSign = 0;
			if (mp.buttons.released(BTN_FUN_LEFT))
				contact.remove(contact.length() - 1);
			if (key != NO_KEY && isdigit(key) && contact.length() < 16)
				contact += key;
			mp.display.setTextWrap(1);
			
			mp.display.setCursor(1*scale, y);
			if (content == "")
			{
				mp.display.setTextColor(TFT_LIGHTGREY);
				mp.display.print(F("Compose..."));
				mp.display.setCursor(1*scale+1, y+14);
				mp.display.print(F("Press A to send"));
				mp.display.setTextColor(TFT_WHITE);
			}
			else
			{
				for(int i = 0; i < content.length();i++)
				{
					mp.display.print(content[i]);
					if(mp.display.getCursorX() > 150)
						mp.display.print("\n");
				}
        		mp.display.fillRect(0, 0, mp.display.width(), 14, TFT_DARKGREY);
			}
						
            mp.display.setCursor(27, -1);

			contactLabel = mp.checkContact(contact);
			if(contactLabel != "")
				mp.display.print(contactLabel);
			else
				mp.display.print(contact);

			if (blinkState == 1)
                mp.display.drawFastVLine(mp.display.getCursorX(), mp.display.getCursorY()+3, 10, TFT_WHITE);
		}
		if ((mp.buttons.released(BTN_UP)) && cursor) { //BUTTON UP
			mp.buttons.update();
			cursor = 0;
		}
		if ((mp.buttons.released(BTN_DOWN)) && !cursor) { //BUTTON DOWN
			mp.buttons.update();
			cursor = 1;
		}

		if (mp.buttons.released(BTN_B)) //BUTTON BACK
		{
			while(!mp.update());
			break;
		}


		if(mp.buttons.released(BTN_FUN_RIGHT) && cursor == 1)
		{
			helpPop = !helpPop;
			mp.display.drawIcon(TextHelperPopup, 0, 0, 160, 128, 1, TFT_WHITE);	
			while(!mp.update());
		}
		while (helpPop) 
		{
			if(mp.buttons.released(BTN_FUN_RIGHT) || mp.buttons.released(BTN_B))
			{
				helpPop = !helpPop;
			}
			mp.update();
		}
		if(mp.buttons.released(BTN_FUN_RIGHT) && cursor == 0 && contactsApp == false)
		{
			while(!mp.update());
			contactsApp = true;
			contactID = 0;
			if (!jarr.success())
				Serial.println("Error loading contacts");
			else if(contactsEmpty)
			{
				mp.display.fillScreen(TFT_BLACK);
				mp.display.setCursor(0,mp.display.height()/2 -16);
				mp.display.printCenter(F("Contacts empty!"));
				uint32_t tempMillis = millis();
				while(millis() < tempMillis + 2000 && !mp.buttons.released(BTN_A) && !mp.buttons.released(BTN_B))
					mp.update();
				while(!mp.update());
			}
			else
			{
				contactID = contactsMenu(&jarr, true); //call contacts app with smsFlag set to true
				contactTemp = jarr[contactID+1]["number"].as<String>();
				Serial.println("contactID");
				Serial.println(contactID);
				contactID = 0;
			}
			if(contactTemp != "")
				contact = contactTemp;
			contactsApp = false;
			
		}
		while(contactsApp) 
		{
			if(mp.buttons.released(BTN_FUN_RIGHT) || mp.buttons.released(BTN_B))
			{
				contactsApp = !contactsApp;
			}
			mp.update();
		}
		if (mp.buttons.released(BTN_A) && contact.length() > 1 && content != "") // SEND SMS
		{
			mp.display.fillScreen(TFT_BLACK);
            mp.display.setCursor(0, mp.display.height()/2 - 16);
            mp.display.setTextFont(2);
			mp.display.printCenter("Sending text...");
			if(contact.startsWith("00"))
				contact = String("+" + contact.substring(2));
			while(!mp.update());
			Serial1.println("AT+CMGF=1");
			mp.waitForOK();

			Serial1.print("AT+CMGS=\"");
			Serial1.print(contact);
			Serial1.println("\"");
			// mp.waitForOK();
			uint32_t temp = millis();
			bool goAhead = 1;
			while (!Serial1.available())
			{
				if(millis()-temp > 1000)
				{
					Serial1.print("AT+CMGS=\"");
					Serial1.print(contact);
					Serial1.println("\"");
				}
				if(millis() - temp > 2000)
				{
					goAhead = 0;
					break;
				}
			}
			if(goAhead)
			{
				Serial.println(Serial1.readString());
				delay(5);
				Serial1.print(content);
				while (!Serial1.available());
				Serial.println(Serial1.readString());
				delay(5);
				Serial1.println((char)26);
				while (Serial1.readString().indexOf("OK") != -1);
				while(!Serial1.available());
				mp.display.fillScreen(TFT_BLACK);
				mp.display.printCenter("Text sent!");
				while(!mp.update());
				// String temp = mp.checkContact(contact);
				mp.saveMessage((char*)content.c_str(), mp.checkContact(contact), contact, 1, 0, mp.RTC.now());
				delay(1000);
			}
			else
			{
				mp.display.fillScreen(TFT_BLACK);
				mp.display.printCenter("Error sending text!");
				while(!mp.update());
				delay(1000);
			}
			Serial.println("cmgf checking");
			Serial1.println("AT+CMGF=0");
			mp.waitForOK();
			bool pduModeCheck = 0;
			uint32_t timeout = millis();
			while(!pduModeCheck && millis() - timeout < 10000)
			{
				Serial1.println("AT+CMGF?");
				String ret = mp.waitForOK();
				Serial.println(ret);
				Serial.println("---------");
				if(ret.indexOf("+CMGF: ") != -1 && ret.substring(ret.indexOf("+CMGF: ") + 7, ret.indexOf("+CMGF: ") + 8) == "0")
					pduModeCheck = 1;
				else if(ret.indexOf("+CMGF: ") != -1 && ret.substring(ret.indexOf("+CMGF: ") + 7, ret.indexOf("+CMGF: ") + 8) != "0")
				{
					Serial1.println("AT+CMGF=0");
					mp.waitForOK();
					delay(1000);
				}
			}
			break;
		}
		mp.display.setTextColor(TFT_WHITE);
        mp.display.setTextFont(2);
        mp.display.setCursor(2,-1);
        mp.display.drawFastHLine(0, 14, mp.display.width(), TFT_WHITE);
		mp.display.print("To: ");
		if(contactLabel != "")
			mp.display.print(contactLabel);
		else
			mp.display.print(contact);
		mp.display.fillRect(0,110, 160, 18, TFT_DARKGREY);
		mp.display.drawFastHLine(0,111, 160, TFT_WHITE);
		mp.display.setCursor(4, 112);
		mp.display.print("Erase");
		if(cursor == 0)
		{
			mp.display.setCursor(102,112);
			mp.display.print("Contacts");
		}
		else if(cursor == 1)
		{
			mp.display.setCursor(130,112);
			mp.display.print("Help");
		}
		mp.update();
		if(mp.newMessage)
		{
			File file = SD.open("/.core/messages.json", "r");
			jb.clear();
			JsonArray& jarr = jb.parseArray(file);
			file.close();
			if(!jarr.success())
			{
				Serial.println("Error");
				mp.display.fillScreen(TFT_BLACK);
				mp.display.setCursor(0, mp.display.height()/2 - 16);
				mp.display.setTextFont(2);
				mp.display.printCenter("Error loading data");

				while (mp.buttons.released(BTN_B) == 0)//BUTTON BACK
					mp.update();
				while(!mp.update());
			}
			mp.newMessage = 0;
		}
	}
}