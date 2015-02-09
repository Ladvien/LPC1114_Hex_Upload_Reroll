unsigned char rx()
{
	static int RX_state;
	int new_RX_state;
	static unsigned char error[32];

	switch(RX_state)
		{	
			case RX_NODATA:
				if (RxBytes > 0)
				{
					new_RX_state = RX_INCOMPLETE;
				}
				break;
			case RX_INCOMPLETE:
				// If RxBuffer contains "\r\n"
				{
					new_RX_state = RX_COMPLETE;	
				}
				break;
			case RX_COMPLETE:
				printf("RX_COMPLETE\n");
				// If RxBuffer contains "\r\n" && data after.
				{
					new_RX_state = RX_COMP_AND_INC_DATA;	
				}
				break;
			case RX_COMP_AND_INC_DATA:
				// Save left over data for next state.
				printf("RX_COMP_AND_INC_DATA\n");
				// If left over data contains "\r\n"
				{
					new_RX_state = RX_COMPLETE;		
				}
				else
				{
					new_RX_state = RX_COMP_AND_INC_DATA;	
				}
				break;
			case RX_ERROR:
				printf("Error: %s\n", error);
				break;
			default:
				printf("Unknown error.");
				break;
		}
		RX_state = new_RX_state;
		return new_RX_state;
}


unsigned char rx()
{
	static int RX_state;
	int new_RX_state;

	static unsigned char error[32];

	switch(RX_state)
		{	
			case RX_NODATA:
				// Check if we Rx'ed any bytes.
				FT_GetQueueStatus(handle,&RxBytes);
				if (RxBytes > 0)
					{
						// We did, let's tell the machine to look over the bytes
						// for completeness.
						new_RX_state = RX_INCOMPLETE;
					}			
				else {
					// We received no data.  Set error code and proceed to error state.
					strncpy(error, "No data to receive.", sizeof("No data to receive."));
					new_RX_state = RX_ERROR;
				}
				break;


			case RX_INCOMPLETE:
				// We have some data, let's see how complete it is.
				// First, we receive it.
				FT_GetStatus(handle, &RxBytes, &TxBytes, &EventDWord);
				FT_status = FT_Read(handle,RxBuffer,RxBytes,&BytesReceived);
				
				//If the read was good.
				if (FT_status == FT_OK) {
					
					// Does the RxBuffer contain an CR LF string?
					unsigned char *CR_LF_CHK = strstr(RxBuffer, "\r\n");
					// If it does contain CR LF
					if(CR_LF_CHK != '\0')
						{
							// Let's replace CR LF with "  ".
							int replaceIndex = 0;
							printf("RX: ");
							while(RxBuffer[replaceIndex] != 0x00)
								{
									if(RxBuffer[replaceIndex] == '\r' || RxBuffer[replaceIndex] == '\n'){putchar(' ');}
									else{{putchar(RxBuffer[replaceIndex]);}									}
									replaceIndex++;
								}
							// Silly, but let's add our own LF.
							printf("\n");
							new_RX_state = RX_INCOMPLETE;
							break;
					}
					// If the RX'ed data does not contain CR LF string.
					else
					{
						// Let's print what's in the buffer and add a LF.
						new_RX_state = RX_COMPLETE;
						printf("RX: %s\n", RxBuffer);
						break;
					}
				}
				else
				{
					strncpy(error, "Problem with reading incomplete data.", sizeof("Problem with reading incomplete data."));
					break;
				}
				//printf("RX_INCOMPLETE\n");
				break;

			case RX_COMPLETE:
				printf("RX_COMPLETE\n");
				break;
			case RX_COMP_AND_INC_DATA:
				printf("RX_COMP_AND_INC_DATA\n");
				break;
			case RX_ERROR:
				printf("Error: %s\n", error);
				break;
			default:
				printf("Unknown error.");
				break;

		}
		
		RX_state = new_RX_state;
		return new_RX_state;
}