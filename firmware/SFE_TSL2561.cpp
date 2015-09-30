/*
	SFE_TSL2561 illumination sensor library for Arduino
	Mike Grusin, SparkFun Electronics
	
	This library provides functions to access the TAOS TSL2561
	Illumination Sensor.
	
	Our example code uses the "beerware" license. You can do anything
	you like with this code. No really, anything. If you find it useful,
	buy me a beer someday.

	version 1.0 2013/09/20 MDG initial version
*/

#include "SFE_TSL2561.h"
#include <math.h>


SFE_TSL2561::SFE_TSL2561(void){
}


boolean SFE_TSL2561::begin(void)
{
	return(begin(TSL2561_ADDR));
}


boolean SFE_TSL2561::begin(char i2c_address)
{
	_i2c_address = i2c_address;
	Wire.begin();
	return(true);
}


boolean SFE_TSL2561::setPowerUp(void)
{
	// Write 0x03 to command byte (power on)
	return(writeByte(TSL2561_REG_CONTROL,0x03));
}


boolean SFE_TSL2561::setPowerDown(void)
	// Turn off TSL2561
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	// Clear command byte (power off)
	return(writeByte(TSL2561_REG_CONTROL,0x00));
}


boolean SFE_TSL2561::setTiming(boolean gain, unsigned char time)
	// If gain = false (0), device is set to low gain (1X)
	// If gain = high (1), device is set to high gain (16X)
	// If time = 0, integration will be 13.7ms
	// If time = 1, integration will be 101ms
	// If time = 2, integration will be 402ms
	// If time = 3, use manual start / stop
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	unsigned char timing;

	// Get timing byte
	if (readByte(TSL2561_REG_TIMING,timing))
	{
		// Set gain (0 or 1)
		if (gain)
			timing |= 0x10;
		else
			timing &= ~0x10;

		// Set integration time (0 to 3)
		timing &= ~0x03;
		timing |= (time & 0x03);

		// Write modified timing byte back to device
		if (writeByte(TSL2561_REG_TIMING,timing))
			return(true);
	}
	return(false);
}


boolean SFE_TSL2561::setTiming(boolean gain, unsigned char time, unsigned int &ms)
	// If gain = false (0), device is set to low gain (1X)
	// If gain = high (1), device is set to high gain (16X)
	// If time = 0, integration will be 13.7ms
	// If time = 1, integration will be 101ms
	// If time = 2, integration will be 402ms
	// If time = 3, use manual start / stop (ms = 0)
	// ms will be set to integration time
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	// Calculate ms for user
	switch (time)
	{
		case 0: ms = 14; break;
		case 1: ms = 101; break;
		case 2: ms = 402; break;
		default: ms = 0;
	}
	// Set integration using base function
	return(setTiming(gain,time));
}


boolean SFE_TSL2561::manualStart(void)
	// Starts a manual integration period
	// After running this command, you must manually stop integration with manualStop()
	// Internally sets integration time to 3 for manual integration (gain is unchanged)
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	unsigned char timing;
	
	// Get timing byte
	if (readByte(TSL2561_REG_TIMING,timing))
	{
		// Set integration time to 3 (manual integration)
		timing |= 0x03;

		if (writeByte(TSL2561_REG_TIMING,timing))
		{
			// Begin manual integration
			timing |= 0x08;

			// Write modified timing byte back to device
			if (writeByte(TSL2561_REG_TIMING,timing))
				return(true);
		}
	}
	return(false);
}


boolean SFE_TSL2561::manualStop(void)
	// Stops a manual integration period
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	unsigned char timing;
	
	// Get timing byte
	if (readByte(TSL2561_REG_TIMING,timing))
	{
		// Stop manual integration
		timing &= ~0x08;

		// Write modified timing byte back to device
		if (writeByte(TSL2561_REG_TIMING,timing))
			return(true);
	}
	return(false);
}


boolean SFE_TSL2561::getData(unsigned int &data0, unsigned int &data1)
	// Retrieve raw integration results
	// data0 and data1 will be set to integration results
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	// Get data0 and data1 out of result registers
	if (readUInt(TSL2561_REG_DATA_0,data0) && readUInt(TSL2561_REG_DATA_1,data1)) 
		return(true);

	return(false);
}


boolean SFE_TSL2561::getLux(unsigned char gain, unsigned int ms, unsigned int CH0, unsigned int CH1, double &lux)
	// Convert raw data to lux
	// gain: 0 (1X) or 1 (16X), see setTiming()
	// ms: integration time in ms, from setTiming() or from manual integration
	// CH0, CH1: results from getData()
	// lux will be set to resulting lux calculation
	// returns true (1) if calculation was successful
	// RETURNS false (0) AND lux = 0.0 IF EITHER SENSOR WAS SATURATED (0XFFFF)
{
	double ratio, d0, d1;

	// Determine if either sensor saturated (0xFFFF)
	// If so, abandon ship (calculation will not be accurate)
	if ((CH0 == 0xFFFF) || (CH1 == 0xFFFF))
	{
		lux = 0.0;
		return(false);
	}

	// Convert from unsigned integer to floating point
	d0 = CH0; d1 = CH1;

	// We will need the ratio for subsequent calculations
	ratio = d1 / d0;

	// Normalize for integration time
	d0 *= (402.0/ms);
	d1 *= (402.0/ms);

	// Normalize for gain
	if (!gain)
	{
		d0 *= 16;
		d1 *= 16;
	}

	// Determine lux per datasheet equations:
	
	if (ratio < 0.5)
	{
		lux = 0.0304 * d0 - 0.062 * d0 * pow(ratio,1.4);
		return(true);
	}

	if (ratio < 0.61)
	{
		lux = 0.0224 * d0 - 0.031 * d1;
		return(true);
	}

	if (ratio < 0.80)
	{
		lux = 0.0128 * d0 - 0.0153 * d1;
		return(true);
	}

	if (ratio < 1.30)
	{
		lux = 0.00146 * d0 - 0.00112 * d1;
		return(true);
	}

	// if (ratio > 1.30)
	lux = 0.0;
	return(true);
}


boolean SFE_TSL2561::setInterruptControl(unsigned char control, unsigned char persist)
	// Sets up interrupt operations
	// If control = 0, interrupt output disabled
	// If control = 1, use level interrupt, see setInterruptThreshold()
	// If persist = 0, every integration cycle generates an interrupt
	// If persist = 1, any value outside of threshold generates an interrupt
	// If persist = 2 to 15, value must be outside of threshold for 2 to 15 integration cycles
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	// Place control and persist bits into proper location in interrupt control register
	if (writeByte(TSL2561_REG_INTCTL,((control | 0B00000011) << 4) & (persist | 0B00001111)))
		return(true);
		
	return(false);
}


boolean SFE_TSL2561::setInterruptThreshold(unsigned int low, unsigned int high)
	// Set interrupt thresholds (channel 0 only)
	// low, high: 16-bit threshold values
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	// Write low and high threshold values
	if (writeUInt(TSL2561_REG_THRESH_L,low) && writeUInt(TSL2561_REG_THRESH_H,high))
		return(true);
		
	return(false);
}


boolean SFE_TSL2561::clearInterrupt(void)
	// Clears an active interrupt
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	// Set up command byte for interrupt clear
	Wire.beginTransmission(_i2c_address);
	Wire.write(TSL2561_CMD_CLEAR);
	_error = Wire.endTransmission();
	if (_error == 0)
		return(true);

	return(false);
}


boolean SFE_TSL2561::getID(unsigned char &ID)
	// Retrieves part and revision code from TSL2561
	// Sets ID to part ID (see datasheet)
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() below)
{
	// Get ID byte from ID register
	if (readByte(TSL2561_REG_ID,ID))
		return(true);

	return(false);
}


byte SFE_TSL2561::getError(void)
	// If any library command fails, you can retrieve an extended
	// error code using this command. Errors are from the wire library: 
	// 0 = Success
	// 1 = Data too long to fit in transmit buffer
	// 2 = Received NACK on transmit of address
	// 3 = Received NACK on transmit of data
	// 4 = Other error
{
	return(_error);
}

// Private functions:

boolean SFE_TSL2561::readByte(unsigned char address, unsigned char &value)
	// Reads a byte from a TSL2561 address
	// Address: TSL2561 address (0 to 15)
	// Value will be set to stored byte
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)
{
	// Set up command byte for read
	Wire.beginTransmission(_i2c_address);
	Wire.write((address & 0x0F) | TSL2561_CMD);
	_error = Wire.endTransmission();

	// Read requested byte
	if (_error == 0)
	{
		Wire.requestFrom(_i2c_address,1);
		if (Wire.available() == 1)
		{
			value = Wire.read();
			return(true);
		}
	}
	return(false);
}


boolean SFE_TSL2561::writeByte(unsigned char address, unsigned char value)
	// Write a byte to a TSL2561 address
	// Address: TSL2561 address (0 to 15)
	// Value: byte to write to address
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)
{
	// Set up command byte for write
	Wire.beginTransmission(_i2c_address);
	Wire.write((address & 0x0F) | TSL2561_CMD);
	// Write byte
	Wire.write(value);
	_error = Wire.endTransmission();
	if (_error == 0)
		return(true);

	return(false);
}


boolean SFE_TSL2561::readUInt(unsigned char address, unsigned int &value)
	// Reads an unsigned integer (16 bits) from a TSL2561 address (low byte first)
	// Address: TSL2561 address (0 to 15), low byte first
	// Value will be set to stored unsigned integer
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)
{
	char high, low;
	
	// Set up command byte for read
	Wire.beginTransmission(_i2c_address);
	Wire.write((address & 0x0F) | TSL2561_CMD);
	_error = Wire.endTransmission();

	// Read two bytes (low and high)
	if (_error == 0)
	{
		Wire.requestFrom(_i2c_address,2);
		if (Wire.available() == 2)
		{
			low = Wire.read();
			high = Wire.read();
			// Combine bytes into unsigned int
			value = (high << 8) | low;
			return(true);
		}
	}	
	return(false);
}


boolean SFE_TSL2561::writeUInt(unsigned char address, unsigned int value)
	// Write an unsigned integer (16 bits) to a TSL2561 address (low byte first)
	// Address: TSL2561 address (0 to 15), low byte first
	// Value: unsigned int to write to address
	// Returns true (1) if successful, false (0) if there was an I2C error
	// (Also see getError() above)
{
	// Split int into lower and upper bytes, write each byte
	if (writeByte(address,(value & 0xFF)) 
		&& writeByte(address + 1,(value >> 8)))
		return(true);

	return(false);
}
