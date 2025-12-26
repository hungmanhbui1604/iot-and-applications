const express = require('express');
const mongoose = require('mongoose');
const swaggerJsDoc = require('swagger-jsdoc');
const swaggerUi = require('swagger-ui-express');
const cors = require('cors');

const app = express();
app.use(express.json());
app.use(cors());

// --- 1. Database Connection ---
// Replace with your actual MongoDB connection string
const MONGO_URI = 'mongodb://localhost:27017/sensorDb';

mongoose.connect(MONGO_URI)
  .then(() => console.log('✅ Connected to MongoDB'))
  .catch(err => console.error('❌ Could not connect to MongoDB:', err));

// --- 2. Mongoose Models (Translating C# classes to Schemas) ---

// SensorData Schema
const sensorDataSchema = new mongoose.Schema({
    sensorName: { type: String, required: true },
    sensorValue: { type: Number, required: true }
    // Note: MongoDB generates a unique '_id' automatically. 
    // If you strictly need an integer 'id', you would need a separate counter logic.
});
const SensorData = mongoose.model('SensorData', sensorDataSchema);

// User Schema
const userSchema = new mongoose.Schema({
    userName: { type: String, required: true },
    password: { type: String, required: true },
    email: { type: String, required: true }
});
const User = mongoose.model('User', userSchema);

// --- 3. Swagger Configuration ---
const swaggerOptions = {
    swaggerDefinition: {
        openapi: '3.0.0',
        info: {
            title: 'IoT & User API',
            version: '1.0.0',
            description: 'API for managing SensorData and Users',
            contact: {
                name: 'Developer'
            }
        },
        servers: [
            { url: 'http://localhost:3000' }
        ],
        components: {
            schemas: {
                SensorData: {
                    type: 'object',
                    properties: {
                        _id: { type: 'string', description: 'Auto-generated Mongo ID' },
                        sensorName: { type: 'string' },
                        sensorValue: { type: 'number', format: 'float' }
                    }
                },
                User: {
                    type: 'object',
                    properties: {
                        _id: { type: 'string', description: 'Auto-generated Mongo ID' },
                        userName: { type: 'string' },
                        password: { type: 'string' },
                        email: { type: 'string' }
                    }
                }
            }
        }
    },
    apis: ['server.js'], // Tells Swagger to look in this file for annotations
};

const swaggerDocs = swaggerJsDoc(swaggerOptions);
app.use('/api-docs', swaggerUi.serve, swaggerUi.setup(swaggerDocs));

// --- 4. API Routes ---

/**
 * @swagger
 * tags:
 *   - name: SensorData
 *     description: Operations for Sensor Data
 *   - name: Users
 *     description: Operations for User Management
 */

// ==========================
// SensorData Endpoints
// ==========================

/**
 * @swagger
 * /api/sensors:
 *   get:
 *     summary: Get all sensor data
 *     tags: [SensorData]
 *     responses:
 *       '200':
 *         description: List of sensor data
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/SensorData'
 */
app.get('/api/sensors', async (req, res) => {
    try {
        const sensors = await SensorData.find();
        res.json(sensors);
    } catch (err) {
        res.status(500).json({ message: err.message });
    }
});

/**
 * @swagger
 * /api/sensors:
 *   post:
 *     summary: Create new sensor data
 *     tags: [SensorData]
 *     requestBody:
 *       required: true
 *       content:
 *         application/json:
 *           schema:
 *             type: object
 *             properties:
 *               sensorName:
 *                 type: string
 *               sensorValue:
 *                 type: number
 *     responses:
 *       '201':
 *         description: Created successfully
 */
app.post('/api/sensors', async (req, res) => {
    const sensor = new SensorData({
        sensorName: req.body.sensorName,
        sensorValue: req.body.sensorValue
    });

    try {
        const newSensor = await sensor.save();
        res.status(201).json(newSensor);
    } catch (err) {
        res.status(400).json({ message: err.message });
    }
});

/**
 * @swagger
 * /api/sensors/{id}:
 *   put:
 *     summary: Update sensor data by ID
 *     tags: [SensorData]
 *     parameters:
 *       - in: path
 *         name: id
 *         required: true
 *         schema:
 *           type: string
 *     requestBody:
 *       required: true
 *       content:
 *         application/json:
 *           schema:
 *             $ref: '#/components/schemas/SensorData'
 *     responses:
 *       '200':
 *         description: Updated successfully
 */
app.put('/api/sensors/:id', async (req, res) => {
    try {
        const updatedSensor = await SensorData.findByIdAndUpdate(
            req.params.id, 
            req.body, 
            { new: true }
        );
        res.json(updatedSensor);
    } catch (err) {
        res.status(400).json({ message: err.message });
    }
});

/**
 * @swagger
 * /api/sensors/{id}:
 *   delete:
 *     summary: Delete sensor data by ID
 *     tags: [SensorData]
 *     parameters:
 *       - in: path
 *         name: id
 *         required: true
 *         schema:
 *           type: string
 *     responses:
 *       '200':
 *         description: Deleted successfully
 */
app.delete('/api/sensors/:id', async (req, res) => {
    try {
        await SensorData.findByIdAndDelete(req.params.id);
        res.json({ message: 'Deleted Sensor Data' });
    } catch (err) {
        res.status(500).json({ message: err.message });
    }
});

// ==========================
// User Endpoints
// ==========================

/**
 * @swagger
 * /api/users:
 *   get:
 *     summary: Get all users
 *     tags: [Users]
 *     responses:
 *       '200':
 *         description: List of users
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/User'
 */
app.get('/api/users', async (req, res) => {
    try {
        const users = await User.find();
        res.json(users);
    } catch (err) {
        res.status(500).json({ message: err.message });
    }
});

/**
 * @swagger
 * /api/users:
 *   post:
 *     summary: Create a new user
 *     tags: [Users]
 *     requestBody:
 *       required: true
 *       content:
 *         application/json:
 *           schema:
 *             type: object
 *             properties:
 *               userName:
 *                 type: string
 *               password:
 *                 type: string
 *               email:
 *                 type: string
 *     responses:
 *       '201':
 *         description: User created
 */
app.post('/api/users', async (req, res) => {
    const user = new User({
        userName: req.body.userName,
        password: req.body.password,
        email: req.body.email
    });

    try {
        const newUser = await user.save();
        res.status(201).json(newUser);
    } catch (err) {
        res.status(400).json({ message: err.message });
    }
});

/**
 * @swagger
 * /api/users/{id}:
 *   put:
 *     summary: Update a user
 *     tags: [Users]
 *     parameters:
 *       - in: path
 *         name: id
 *         required: true
 *         schema:
 *           type: string
 *     requestBody:
 *       required: true
 *       content:
 *         application/json:
 *           schema:
 *             $ref: '#/components/schemas/User'
 *     responses:
 *       '200':
 *         description: User updated
 */
app.put('/api/users/:id', async (req, res) => {
    try {
        const updatedUser = await User.findByIdAndUpdate(
            req.params.id, 
            req.body, 
            { new: true }
        );
        res.json(updatedUser);
    } catch (err) {
        res.status(400).json({ message: err.message });
    }
});

/**
 * @swagger
 * /api/users/{id}:
 *   delete:
 *     summary: Delete a user
 *     tags: [Users]
 *     parameters:
 *       - in: path
 *         name: id
 *         required: true
 *         schema:
 *           type: string
 *     responses:
 *       '200':
 *         description: User deleted
 */
app.delete('/api/users/:id', async (req, res) => {
    try {
        await User.findByIdAndDelete(req.params.id);
        res.json({ message: 'Deleted User' });
    } catch (err) {
        res.status(500).json({ message: err.message });
    }
});

// --- 5. Start Server ---
const PORT = 3000;
app.listen(PORT, () => {
    console.log(`🚀 Server running on http://localhost:${PORT}`);
    console.log(`📄 Swagger UI available at http://localhost:${PORT}/api-docs`);
});