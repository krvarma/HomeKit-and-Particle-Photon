var Service, Characteristic;
var Request = require("request");
var EventSource = require('eventsource');

var temperatureService;
var lightSensorService;
var humiditySensor;
var bulbService;
var bulbServiceHandling;
var roomName;

var eventName = "HKSValues";

var url;
var deviceid;
var accesstoken;

module.exports = function (homebridge) {
    Service = homebridge.hap.Service;
    Characteristic = homebridge.hap.Characteristic;
    homebridge.registerAccessory("homebridge-particle-photon", "ParticleAccessory", ParticleAccessory);
}

function ParticleAccessory(log, config) {
	this.log = log;

	// url info
	url = config["cloud_url"];
	deviceid = config["deviceid"];
	accesstoken = config["accesstoken"];
	bulbServiceHandling = config["bulb_service"];
	roomName = config["room_name"];
}

function random(low, high) {
	return Math.random() * (high - low) + low;
}

ParticleAccessory.prototype = {
	identify: function(callback) {
		this.log("Identify requested!");
		callback(); // success
	},

	getDefaultValue: function(callback) {
		callback(null, 0.0);
	},

	setBulbState: function(state, callback) {
		var setLightOnUrl = url + deviceid + "/ctrllight";

		Request.post(
			setLightOnUrl, {
				form: {
					access_token: accesstoken,
					args: (state ? 1 : 0)
				}
			},
			function(error, response, body) {
				// If not error then prepare message and send

				console.log(response);

				if (!error) {
					callback();
				} else {
					callback(error);
				}
			}
		);
	},

	setBrightness: function(level, callback) {
		console.log(level);

		var setLightBrightnessUrl = url + deviceid + "/brightness";

		Request.post(
			setLightBrightnessUrl, {
				form: {
					access_token: accesstoken,
					args: level
				}
			},
			function(error, response, body) {
				// If not error then prepare message and send

				console.log(response);

				if (!error) {
					callback();
				} else {
					callback(error);
				}
			}
		);
	},

	setHue: function(value, callback) {
		console.log(value);

		var setLightHueUrl = url + deviceid + "/sethue";

		Request.post(
			setLightHueUrl, {
				form: {
					access_token: accesstoken,
					args: value
				}
			},
			function(error, response, body) {
				// If not error then prepare message and send

				console.log(response);

				if (!error) {
					callback();
				} else {
					callback(error);
				}
			}
		);
	},

	HSVtoRGB: function(h, s, v) {
		while (h < 0) {
			h += 360;
		}
		i = (h / 60 >> 0) % 6;
		f = h / 60 - i;
		v *= 255;
		p = v * (1 - s);
		q = v * (1 - f * s);
		t = v * (1 - (1 - f) * s);
		switch (i) {
			case 0:
				r = v;
				g = t;
				b = p;
				break;
			case 1:
				r = q;
				g = v;
				b = p;
				break;
			case 2:
				r = p;
				g = v;
				b = t;
				break;
			case 3:
				r = p;
				g = q;
				b = v;
				break;
			case 4:
				r = t;
				g = p;
				b = v;
				break;
			case 5:
				r = v;
				g = p;
				b = q;
		}

		return [r, g, b];
	},

	getServices: function() {
		// you can OPTIONALLY create an information service if you wish to override
		// the default values for things like serial number, model, etc.
		var informationService = new Service.AccessoryInformation();

		informationService
			.setCharacteristic(Characteristic.Manufacturer, "Particle")
			.setCharacteristic(Characteristic.Model, "Photon")
			.setCharacteristic(Characteristic.SerialNumber, "AA098BB09");

		temperatureService = new Service.TemperatureSensor(roomName + " Temperature");

		temperatureService
			.getCharacteristic(Characteristic.CurrentTemperature)
			.on('get', this.getDefaultValue.bind(this));

		lightSensorService = new Service.LightSensor(roomName + " Light Sensor");

		lightSensorService
			.getCharacteristic(Characteristic.CurrentAmbientLightLevel)
			.on('get', this.getDefaultValue.bind(this));

		humiditySensor = new Service.HumiditySensor();

		humiditySensor
			.getCharacteristic(Characteristic.CurrentRelativeHumidity)
			.on('get', this.getDefaultValue.bind(this));

		if (this.bulbServiceHandling == "yes") {
			bulbService = new Service.Lightbulb(roomName + " Light");

			bulbService
				.getCharacteristic(Characteristic.On)
				.on('set', this.setBulbState.bind(this));

			bulbService
				.setCharacteristic(Characteristic.Name, roomName + " Light");

			bulbService
				.addCharacteristic(new Characteristic.Brightness())
				.on('set', this.setBrightness.bind(this));

			bulbService
				.addCharacteristic(new Characteristic.Hue())
				.on('set', this.setHue.bind(this));
		}

		var eventUrl = url + deviceid + "/events/" + eventName + "?access_token=" + accesstoken;

		console.log(eventUrl);

		var es = new EventSource(eventUrl);

		es.onerror = function() {
			console.log('ERROR!');
		};

		es.addEventListener(eventName,
			function(e) {
				var data = JSON.parse(e.data);
				var tokens = data.data.split('=');

				//console.log(tokens);

				if (tokens[0].toLowerCase() === "temperature") {
					console.log("Temperature " + tokens[1] + " C");

					temperatureService
						.setCharacteristic(Characteristic.CurrentTemperature, parseFloat(tokens[1]));
				} else if (tokens[0].toLowerCase() === "lux") {
					console.log("Light " + tokens[1] + " lux");

					lightSensorService
						.setCharacteristic(Characteristic.CurrentAmbientLightLevel, parseFloat(tokens[1]));
				} else if (tokens[0].toLowerCase() === "humidity") {
					console.log("Humidity " + tokens[1] + "%");

					humiditySensor
						.setCharacteristic(Characteristic.CurrentRelativeHumidity, parseFloat(tokens[1]));
				}

				//console.log(data.data);
			}, false);

		if (this.bulbServiceHandling == "yes") {
			return [informationService, temperatureService, lightSensorService, humiditySensor, bulbService];
		}else{
			return [informationService, temperatureService, lightSensorService, humiditySensor];
		}
	}
};
