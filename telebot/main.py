import logging
import asyncio
import html
import paho.mqtt.client as mqtt
from telegram import Update
from telegram.ext import ApplicationBuilder, ContextTypes, CommandHandler

# ==========================================
#              CONFIGURATION
# ==========================================

# Telegram Settings
TELEGRAM_TOKEN = "" 
AUTHORIZED_CHAT_ID = 0

# MQTT Settings
MQTT_BROKER = ""
MQTT_PORT = 1883
MQTT_USERNAME = ""
MQTT_PASSWORD = ""

TOPIC_SUBSCRIBE = "test/topic1"  # Bot listens to this
TOPIC_PUBLISH = "test/topic2"    # Bot sends commands to this

# ==========================================
#              LOGGING
# ==========================================
logging.basicConfig(
    format='%(asctime)s - %(levelname)s - %(message)s',
    level=logging.INFO
)
logger = logging.getLogger(__name__)

main_loop = None
bot_app = None

# ==========================================
#           MQTT FUNCTIONS
# ==========================================

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        logger.info("✅ Connected to MQTT Broker!")
        client.subscribe(TOPIC_SUBSCRIBE)
    else:
        logger.error(f"❌ Failed to connect to MQTT, return code {rc}")

def on_message(client, userdata, msg):
    """Handles incoming MQTT messages (Sensor -> Telegram)"""
    try:
        payload_str = msg.payload.decode()
        logger.info(f"📨 MQTT Received: {payload_str}")
        
        # Prepare message for Telegram
        safe_payload = html.escape(payload_str)
        text_msg = f"🔔 <b>MQTT Update:</b>\n{safe_payload}"

        if main_loop and bot_app:
            asyncio.run_coroutine_threadsafe(
                bot_app.bot.send_message(chat_id=AUTHORIZED_CHAT_ID, text=text_msg, parse_mode="HTML"),
                main_loop
            )
    except Exception as e:
        logger.error(f"Error forwarding message: {e}")

# ==========================================
#         TELEGRAM COMMAND FUNCTIONS
# ==========================================

async def start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Runs when you type /start"""
    await context.bot.send_message(
        chat_id=update.effective_chat.id, 
        text=f"✅ Bot Online.\n\nType /on to turn device ON.\nType /off to turn device OFF."
    )

async def open_relay(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Runs when you type /on"""
    # 1. Publish to MQTT
    mqtt_client.publish(TOPIC_PUBLISH, "ON")
    logger.info(f"📤 Published 'ON' to {TOPIC_PUBLISH}")
    
    # 2. Reply to Telegram User
    await context.bot.send_message(
        chat_id=update.effective_chat.id, 
        text=f"⚡ <b>Command Sent:</b> ON sent to <code>{TOPIC_PUBLISH}</code>",
        parse_mode="HTML"
    )

async def close_relay(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Runs when you type /off"""
    # 1. Publish to MQTT
    mqtt_client.publish(TOPIC_PUBLISH, "OFF")
    logger.info(f"📤 Published 'OFF' to {TOPIC_PUBLISH}")
    
    # 2. Reply to Telegram User
    await context.bot.send_message(
        chat_id=update.effective_chat.id, 
        text=f"🌑 <b>Command Sent:</b> OFF sent to <code>{TOPIC_PUBLISH}</code>",
        parse_mode="HTML"
    )

# ==========================================
#              MAIN EXECUTION
# ==========================================

if __name__ == '__main__':
    # 1. Kill old processes first!
    # If using Linux/Mac, run: pkill -f main.py
    
    # 2. Setup MQTT
    mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    
    if MQTT_USERNAME and MQTT_PASSWORD:
        mqtt_client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    try:
        mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        mqtt_client.loop_start() # Run MQTT in background thread
    except Exception as e:
        logger.error(f"Cannot connect to broker: {e}")
        exit(1)

    # 3. Setup Telegram
    bot_app = ApplicationBuilder().token(TELEGRAM_TOKEN).build()
    
    # REGISTER COMMANDS HERE
    bot_app.add_handler(CommandHandler('start', start))
    bot_app.add_handler(CommandHandler('/open-relay', open_relay))   # <--- LISTENS FOR /on
    bot_app.add_handler(CommandHandler('/close-relay', close_relay)) # <--- LISTENS FOR /off
    
    # 4. Save Main Loop (needed for thread safety)
    main_loop = asyncio.get_event_loop()

    logger.info("🤖 Bot is running. Open Telegram and type /on")
    bot_app.run_polling()