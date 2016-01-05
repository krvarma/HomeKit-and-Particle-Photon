Copy the directory **homebridge-particle-photon** of this repo to your **/usr/local/lib/node_modules/** directory on RPi

In /usr/local/lib/node_modules/homebridge-particle-photon, create a dir **node_modules**

Then install locally eventsource and request :
npm install eventsource

npm install request

Configure Homebridge to use your ParticleAccessorry, and restart !