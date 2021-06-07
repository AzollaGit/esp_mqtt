# Git flow
{
	…or create a new repository on the command line
	echo "# esp_mqtt" >> README.md    
	git init  
	git add README.md
	git commit -m "first commit"
	git remote add origin https://github.com/AzollaGit/esp_mqtt.git
	git push -u origin master

	…or push an existing repository from the command line
	git remote add origin https://github.com/AzollaGit/esp_mqtt.git
	git push -u origin master
}

# Native OTA example

This example is based on `app_update` component's APIs.

## Configuration

Refer the README.md in the parent directory for the setup details.