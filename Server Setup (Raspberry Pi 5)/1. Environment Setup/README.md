# Environment Setup (Raspberry Pi 5)

This step prepares the Raspberry Pi for:

- Django web application
- PostgreSQL database
- MQTT broker (Mosquitto)
- uWSGI (application server)
- Nginx (web server / reverse proxy)

> **Project name:** `poultry_farm`  
> **Hostname:** `kali`

### Update system

```bash
sudo apt update
sudo apt full-upgrade -y
sudo apt autoremove -y
sudo apt clean
```

### Install basic tools

```bash
sudo apt install -y git curl wget vim nano htop tree unzip zip jq net-tools lsof tmux
```

### Install Python environment packages

```bash
sudo apt install -y python3 python3-pip python3-venv python3-dev
sudo apt install -y build-essential pkg-config
sudo apt install -y libpq-dev libffi-dev libssl-dev
```

### Install web server (Nginx)

```bash
sudo apt install -y nginx
sudo systemctl enable nginx
sudo systemctl start nginx
sudo systemctl status nginx
```

> **Test:** Open in browser
>
> **http://pi-ip** (example: `http://192.168.0.101`)

You should see the **Nginx default page**.

### Install PostgreSQL (database server)

```bash
sudo apt install -y postgresql postgresql-contrib
sudo systemctl enable postgresql
sudo systemctl start postgresql
sudo systemctl status postgresql
```

### Install MQTT broker (Mosquitto)

```bash
sudo apt install -y mosquitto mosquitto-clients
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
sudo systemctl status mosquitto
```

### Quick test

**Terminal 1:**

```bash
mosquitto_sub -h localhost -t test/topic
```

**Terminal 2:**

```bash
mosquitto_pub -h localhost -t test/topic -m "hello"
```

If everything is working, Terminal 1 should receive:

```txt
hello
```

### Create project folder

```bash
sudo mkdir -p /var/www/poultry_farm
sudo chown -R $USER:$USER /var/www/poultry_farm
cd /var/www/poultry_farm
```

### Create Python virtual environment

```bash
python3 -m venv venv
source venv/bin/activate
python --version
pip install --upgrade pip wheel
```

> ⚠️ Every time you open a new terminal, activate it again:
```bash
cd /var/www/poultry_farm
source venv/bin/activate
```

### Install Python packages (base)

```bash
pip install django
pip install uwsgi
pip install psycopg[binary]
pip install paho-mqtt
pip install djangorestframework python-dotenv
```

#### Package use

- `django` → main web framework
- `uwsgi` → app server (used later with Nginx)
- `psycopg[binary]` → PostgreSQL connector for Django
- `paho-mqtt` → MQTT subscriber/publisher client
- `djangorestframework` → REST API support
- `python-dotenv` → load environment variables from `.env`

### Create logs folder

```bash
sudo mkdir -p /var/log/poultry_farm
sudo chown -R $USER:$USER /var/log/poultry_farm
```

### Remove default Nginx site

```bash
sudo rm -f /etc/nginx/sites-enabled/default
sudo systemctl restart nginx
```

### Final check (services running)

```bash
systemctl status nginx --no-pager
systemctl status postgresql --no-pager
systemctl status mosquitto --no-pager
```

#### Check project folder structure:

```bash
tree -L 2 /var/www/poultry_farm
```

#### Expected (basic):

```txt
/var/www/poultry_farm
└── venv
```