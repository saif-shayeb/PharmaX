#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);
HardwareSerial& arduinoSerial = Serial2;

const char* ssid = "PharmacySystem";
const char* password = "12345678";

// Medicine database
struct Medicine {
  String name;
  int shelfRow;
  int shelfCol;
  int stock;
};

Medicine medicines[12] = {
  {"Paracetamol 500mg", 0, 0, 50},
  {"Ibuprofen 400mg", 0, 1, 30},
  {"Amoxicillin 250mg", 0, 2, 40},
  {"Omeprazole 20mg", 0, 3, 25},
  {"Aspirin 100mg", 1, 0, 60},
  {"Cetirizine 10mg", 1, 1, 45},
  {"Metformin 500mg", 1, 2, 35},
  {"Atorvastatin 20mg", 1, 3, 20},
  {"Salbutamol Inhaler", 2, 0, 15},
  {"Loratadine 10mg", 2, 1, 30},
  {"Diazepam 5mg", 2, 2, 10},
  {"Ciprofloxacin 500mg", 2, 3, 25}
};

struct OrderItem {
  int medicineId;
  String medicineName;
  String shelfLocation;
  int quantity;
  String status;
  int processedCount;
};

OrderItem currentOrder[10];
int orderSize = 0;
bool isProcessingOrder = false;
bool systemPaused = false;

void setup() {
  Serial.begin(115200);
  arduinoSerial.begin(115200, SERIAL_8N1, 16, 17);
  
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Wait for Arduino to initialize
  Serial.println("Waiting for Arduino...");
  while (!arduinoSerial.available()) {
    delay(100);
  }
  String response = arduinoSerial.readStringUntil('\n');
  if (response.startsWith("ARDUINO_READY")) {
    Serial.println("Arduino ready");
  }

  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Pharmacy Robot</title>
  <link href="https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css">
  <style>
    :root {
      --primary: #4361ee;
      --secondary: #3f37c9;
      --accent: #4895ef;
      --danger: #f72585;
      --success: #4cc9f0;
      --warning: #f8961e;
      --dark: #212529;
      --light: #f8f9fa;
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      font-family: 'Poppins', sans-serif;
      background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
      min-height: 100vh;
      padding: 20px;
      color: var(--dark);
    }
    
    .container {
      max-width: 1200px;
      margin: 0 auto;
      background: white;
      border-radius: 16px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.1);
      overflow: hidden;
      display: flex;
      flex-direction: column;
      min-height: 90vh;
    }
    
    header {
      background: linear-gradient(135deg, var(--primary) 0%, var(--secondary) 100%);
      color: white;
      padding: 20px 30px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 15px;
    }
    
    h1 {
      font-weight: 600;
      font-size: 1.8rem;
    }
    
    .status-bar {
      background: rgba(255,255,255,0.2);
      padding: 10px 15px;
      border-radius: 50px;
      font-size: 0.9rem;
      display: flex;
      align-items: center;
    }
    
    .status-dot {
      width: 10px;
      height: 10px;
      background: #4ade80;
      border-radius: 50%;
      margin-right: 8px;
      animation: pulse 2s infinite;
    }
    
    @keyframes pulse {
      0% { transform: scale(1); }
      50% { transform: scale(1.2); }
      100% { transform: scale(1); }
    }
    
    .control-buttons {
      display: flex;
      gap: 10px;
      padding: 15px 30px;
      background: var(--light);
      border-bottom: 1px solid rgba(0,0,0,0.05);
      flex-wrap: wrap;
    }
    
    .btn {
      padding: 10px 20px;
      border: none;
      border-radius: 8px;
      font-family: 'Poppins', sans-serif;
      font-weight: 500;
      cursor: pointer;
      transition: all 0.3s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }
    
    .btn-primary {
      background: var(--primary);
      color: white;
      box-shadow: 0 4px 6px rgba(67, 97, 238, 0.2);
    }
    
    .btn-primary:hover {
      background: var(--secondary);
      transform: translateY(-2px);
      box-shadow: 0 6px 12px rgba(67, 97, 238, 0.3);
    }
    
    .btn-warning {
      background: var(--warning);
      color: white;
      box-shadow: 0 4px 6px rgba(248, 150, 30, 0.2);
    }
    
    .btn-warning:hover {
      background: #f9841e;
      transform: translateY(-2px);
      box-shadow: 0 6px 12px rgba(248, 150, 30, 0.3);
    }
    
    .btn-success {
      background: var(--success);
      color: white;
      box-shadow: 0 4px 6px rgba(76, 201, 240, 0.2);
    }
    
    .btn-success:hover {
      background: #3ab8db;
      transform: translateY(-2px);
      box-shadow: 0 6px 12px rgba(76, 201, 240, 0.3);
    }

    .btn-danger {
      background: var(--danger);
      color: white;
      box-shadow: 0 4px 6px rgba(247, 37, 133, 0.2);
    }
    
    .btn-danger:hover {
      background: #f50a72;
      transform: translateY(-2px);
      box-shadow: 0 6px 12px rgba(247, 37, 133, 0.3);
    }
    
    .main-content {
      padding: 30px;
      flex: 1;
    }
    
    .search-container {
      margin-bottom: 25px;
      position: relative;
    }
    
    .search-input {
      width: 100%;
      padding: 12px 20px;
      padding-left: 45px;
      border: 1px solid #dee2e6;
      border-radius: 8px;
      font-family: 'Poppins', sans-serif;
      font-size: 1rem;
      transition: all 0.3s;
      background: var(--light);
    }
    
    .search-input:focus {
      outline: none;
      border-color: var(--accent);
      box-shadow: 0 0 0 3px rgba(72, 149, 239, 0.2);
    }
    
    .search-icon {
      position: absolute;
      left: 15px;
      top: 50%;
      transform: translateY(-50%);
      color: #6c757d;
    }
    
    .medicine-grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(240px, 1fr));
      gap: 20px;
      margin-bottom: 30px;
    }
    
    .medicine-card {
      background: white;
      border-radius: 12px;
      overflow: hidden;
      box-shadow: 0 4px 6px rgba(0,0,0,0.05);
      transition: all 0.3s ease;
      border: 1px solid rgba(0,0,0,0.05);
      display: flex;
      flex-direction: column;
    }
    
    .medicine-card:hover {
      transform: translateY(-5px);
      box-shadow: 0 10px 20px rgba(0,0,0,0.1);
    }
    
    .medicine-header {
      background: linear-gradient(135deg, var(--accent) 0%, var(--primary) 100%);
      color: white;
      padding: 15px;
      display: flex;
      align-items: center;
      gap: 12px;
    }
    
    .medicine-icon {
      font-size: 1.5rem;
      width: 40px;
      height: 40px;
      display: flex;
      align-items: center;
      justify-content: center;
      background: rgba(255,255,255,0.2);
      border-radius: 8px;
    }
    
    .medicine-name {
      font-weight: 600;
      font-size: 1.1rem;
      margin-bottom: 5px;
    }
    
    .medicine-shelf {
      font-size: 0.8rem;
      opacity: 0.9;
    }
    
    .medicine-body {
      padding: 15px;
      flex: 1;
      display: flex;
      flex-direction: column;
    }
    
    .medicine-stock {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 15px;
    }
    
    .stock-count {
      font-weight: 600;
      color: var(--primary);
    }
    
    .stock-bar {
      height: 6px;
      background: #e9ecef;
      border-radius: 3px;
      margin-top: 5px;
      overflow: hidden;
    }
    
    .stock-progress {
      height: 100%;
      background: linear-gradient(90deg, var(--success) 0%, var(--accent) 100%);
      border-radius: 3px;
      transition: width 0.5s ease;
    }
    
    .medicine-controls {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-top: auto;
    }
    
    .quantity-selector {
      padding: 8px 12px;
      border: 1px solid #dee2e6;
      border-radius: 8px;
      font-family: 'Poppins', sans-serif;
      width: 70px;
      background: var(--light);
    }
    
    .add-btn {
      flex: 1;
      padding: 8px 12px;
      background: var(--primary);
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      transition: all 0.3s ease;
      font-family: 'Poppins', sans-serif;
      font-weight: 500;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 5px;
    }
    
    .add-btn:hover {
      background: var(--secondary);
      transform: translateY(-2px);
    }
    
    .add-btn:active {
      transform: translateY(0);
    }
    
    .cart-container {
      background: white;
      border-radius: 12px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.05);
      padding: 20px;
      margin-top: 30px;
      border-top: 3px solid var(--primary);
    }
    
    .cart-title {
      font-size: 1.3rem;
      margin-bottom: 20px;
      color: var(--dark);
      display: flex;
      align-items: center;
      gap: 10px;
    }
    
    .cart-count {
      background: var(--primary);
      color: white;
      width: 24px;
      height: 24px;
      border-radius: 50%;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 0.8rem;
    }
    
    .cart-items {
      max-height: 300px;
      overflow-y: auto;
      padding-right: 10px;
    }
    
    .cart-item {
      padding: 15px 0;
      border-bottom: 1px solid rgba(0,0,0,0.05);
      display: flex;
      justify-content: space-between;
      align-items: center;
      animation: fadeIn 0.3s ease;
    }
    
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(10px); }
      to { opacity: 1; transform: translateY(0); }
    }
    
    .cart-item:last-child {
      border-bottom: none;
    }
    
    .item-info {
      flex: 1;
    }
    
    .item-name {
      font-weight: 500;
      margin-bottom: 5px;
    }
    
    .item-details {
      font-size: 0.8rem;
      color: #6c757d;
      display: flex;
      gap: 10px;
    }
    
    .item-status {
      font-size: 0.8rem;
      padding: 3px 8px;
      border-radius: 4px;
      font-weight: 500;
    }
    
    .status-pending {
      background: rgba(108, 117, 125, 0.1);
      color: #6c757d;
    }
    
    .status-processing {
      background: rgba(248, 150, 30, 0.1);
      color: var(--warning);
    }
    
    .status-completed {
      background: rgba(76, 201, 240, 0.1);
      color: var(--success);
    }
    
    .process-btn {
      width: 100%;
      margin-top: 20px;
      padding: 12px;
      font-size: 1rem;
    }
    
    .empty-cart {
      text-align: center;
      padding: 20px;
      color: #6c757d;
    }

    .item-actions {
      display: flex;
      gap: 8px;
    }

    .remove-btn {
      background: none;
      border: none;
      color: var(--danger);
      cursor: pointer;
      font-size: 1rem;
      transition: all 0.2s ease;
      padding: 5px;
      border-radius: 4px;
    }

    .remove-btn:hover {
      background: rgba(247, 37, 133, 0.1);
      transform: scale(1.1);
    }
    
    /* Animation for add to cart */
    @keyframes bounce {
      0%, 20%, 50%, 80%, 100% {transform: translateY(0);}
      40% {transform: translateY(-10px);}
      60% {transform: translateY(-5px);}
    }
    
    .bounce {
      animation: bounce 0.6s;
    }

    /* Animation for remove */
    @keyframes fadeOut {
      from { opacity: 1; transform: scale(1); }
      to { opacity: 0; transform: scale(0.8); }
    }

    .fade-out {
      animation: fadeOut 0.3s ease forwards;
    }
    
    /* Responsive */
    @media (max-width: 768px) {
      header {
        flex-direction: column;
        text-align: center;
      }
      
      .status-bar {
        margin-top: 10px;
      }
      
      .medicine-grid {
        grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
      }
      
      .control-buttons {
        justify-content: center;
      }
    }
    
    @media (max-width: 480px) {
      .medicine-grid {
        grid-template-columns: 1fr;
      }
      
      .medicine-controls {
        flex-direction: column;
      }
      
      .quantity-selector, .add-btn {
        width: 100%;
      }

      .item-actions {
        flex-direction: column;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1><i class="fas fa-robot"></i> Pharmacy Robot Control</h1>
      <div class="status-bar">
        <div class="status-dot"></div>
        <span id="statusText">System Ready</span>
      </div>
    </header>
    
    <div class="control-buttons">
      <button class="btn btn-primary" onclick="sendCommand('HOME')">
        <i class="fas fa-home"></i> Home Position
      </button>
      <button class="btn btn-warning" id="pauseBtn" onclick="togglePause()">
        <i class="fas fa-pause"></i> Pause
      </button>
      <button class="btn btn-danger" onclick="clearCart()">
        <i class="fas fa-trash"></i> Clear Cart
      </button>
    </div>
    
    <div class="main-content">
      <div class="search-container">
        <i class="fas fa-search search-icon"></i>
        <input type="text" class="search-input" id="searchInput" placeholder="Search medicines..." oninput="filterMedicines()">
      </div>
      
      <h2 style="margin-bottom: 20px;"><i class="fas fa-pills"></i> Medicine Inventory</h2>
      <div class="medicine-grid" id="medicineContainer"></div>
      
      <div class="cart-container">
        <h2 class="cart-title">
          <i class="fas fa-shopping-cart"></i> Current Order
          <span class="cart-count" id="cartCount">0</span>
        </h2>
        
        <div class="cart-items" id="cartItems">
          <div class="empty-cart">No items in cart</div>
        </div>
        
        <button class="btn btn-success process-btn" onclick="processOrder()">
          <i class="fas fa-cogs"></i> Process Order
        </button>
      </div>
    </div>
  </div>

  <script>
    let cart = [];
    let isPaused = false;
    const medicines = [
      {id: 0, name: "Paracetamol 500mg", shelf: "0-0", stock: 50, icon: "fa-tablets"},
      {id: 1, name: "Ibuprofen 400mg", shelf: "0-1", stock: 30, icon: "fa-capsules"},
      {id: 2, name: "Amoxicillin 250mg", shelf: "0-2", stock: 40, icon: "fa-prescription-bottle"},
      {id: 3, name: "Omeprazole 20mg", shelf: "0-3", stock: 25, icon: "fa-capsules"},
      {id: 4, name: "Aspirin 100mg", shelf: "1-0", stock: 60, icon: "fa-tablets"},
      {id: 5, name: "Cetirizine 10mg", shelf: "1-1", stock: 45, icon: "fa-pills"},
      {id: 6, name: "Metformin 500mg", shelf: "1-2", stock: 35, icon: "fa-prescription-bottle-alt"},
      {id: 7, name: "Atorvastatin 20mg", shelf: "1-3", stock: 20, icon: "fa-tablets"},
      {id: 8, name: "Salbutamol Inhaler", shelf: "2-0", stock: 15, icon: "fa-lungs"},
      {id: 9, name: "Loratadine 10mg", shelf: "2-1", stock: 30, icon: "fa-pills"},
      {id: 10, name: "Diazepam 5mg", shelf: "2-2", stock: 10, icon: "fa-prescription-bottle"},
      {id: 11, name: "Ciprofloxacin 500mg", shelf: "2-3", stock: 25, icon: "fa-capsules"}
    ];

    function filterMedicines() {
      const searchTerm = document.getElementById('searchInput').value.toLowerCase();
      const medicineCards = document.querySelectorAll('.medicine-card');
      
      medicineCards.forEach(card => {
        const name = card.querySelector('.medicine-name').textContent.toLowerCase();
        if (name.includes(searchTerm)) {
          card.style.display = 'flex';
        } else {
          card.style.display = 'none';
        }
      });
    }

    function addToCart(medId, quantity = null) {
      const med = medicines.find(m => m.id === medId);
      const qty = quantity || parseInt(document.getElementById(`qty-${medId}`).value);
      
      if (qty > med.stock) {
        updateStatus(`Only ${med.stock} ${med.name} available`);
        return;
      }
      
      const existing = cart.find(item => item.id === medId);
      if (existing) {
        existing.quantity += qty;
      } else {
        cart.push({
          id: medId,
          name: med.name,
          shelf: med.shelf,
          quantity: qty,
          status: 'pending',
          processedCount: 0
        });
      }
      
      // Animation effect
      const btn = document.getElementById(`add-btn-${medId}`);
      btn.classList.add('bounce');
      setTimeout(() => btn.classList.remove('bounce'), 600);
      
      updateCart();
      updateStatus(`Added ${qty} ${med.name} to cart`);
    }

    function removeFromCart(index) {
      // Get the item before removing for status message
      const item = cart[index];
      
      // Find the cart item element
      const cartItems = document.getElementById('cartItems');
      const itemElements = cartItems.querySelectorAll('.cart-item');
      const itemElement = itemElements[index];
      
      // Add fade-out animation
      itemElement.classList.add('fade-out');
      
      // Remove from cart after animation completes
      setTimeout(() => {
        cart.splice(index, 1);
        updateCart();
        updateStatus(`Removed ${item.name} from cart`);
      }, 300);
    }

    function clearCart() {
      if (cart.length === 0) {
        updateStatus("Cart is already empty");
        return;
      }
      
      // Add fade-out animation to all items
      const cartItems = document.getElementById('cartItems');
      const itemElements = cartItems.querySelectorAll('.cart-item');
      itemElements.forEach(item => item.classList.add('fade-out'));
      
      // Clear cart after animation completes
      setTimeout(() => {
        cart = [];
        updateCart();
        updateStatus("Cart cleared");
      }, 300);
    }

    function processOrder() {
      if (cart.length === 0) {
        updateStatus("Cart is empty");
        return;
      }
      
      cart[0].status = 'processing';
      updateCart();
      updateStatus("Processing order...");
      processNextItem();
    }

    function processNextItem() {
      if (isPaused || cart.length === 0) return;
      
      const item = cart[0];
      if (item.processedCount >= item.quantity) {
        item.status = 'completed';
        cart.shift();
        updateCart();
        
        if (cart.length > 0) {
          cart[0].status = 'processing';
          updateStatus(`Processing next item: ${cart[0].name}`);
          setTimeout(processNextItem, 1000);
        } else {
          updateStatus("Order completed successfully!");
        }
        return;
      }
      
      item.processedCount++;
      updateCart();
  sendCommand(`S${item.shelf}`);
      
      setTimeout(() => {
        if (!isPaused) {
          sendCommand("HOME");
          setTimeout(processNextItem, 2000);
        }
      }, 2000);
    }

    function updateCart() {
      const container = document.getElementById('cartItems');
      const countElement = document.getElementById('cartCount');
      
      countElement.textContent = cart.length;
      
      if (cart.length === 0) {
        container.innerHTML = '<div class="empty-cart">No items in cart</div>';
        return;
      }
      
      let html = '';
      cart.forEach((item, index) => {
        const progress = Math.round((item.processedCount / item.quantity) * 100);
        html += `
          <div class="cart-item">
            <div class="item-info">
              <div class="item-name">${item.name}</div>
              <div class="item-details">
                <span>Qty: ${item.quantity}</span>
                <span>Shelf: ${item.shelf}</span>
              </div>
              <div class="item-status status-${item.status}">
                ${item.status === 'processing' 
                  ? `Processing (${progress}%)` 
                  : item.status.charAt(0).toUpperCase() + item.status.slice(1)}
              </div>
            </div>
            <div class="item-actions">
              <button class="remove-btn" onclick="removeFromCart(${index})" title="Remove item">
                <i class="fas fa-times"></i>
              </button>
            </div>
          </div>`;
      });
      container.innerHTML = html;
    }

    function togglePause() {
      isPaused = !isPaused;
      const btn = document.getElementById('pauseBtn');
      btn.innerHTML = isPaused ? '<i class="fas fa-play"></i> Resume' : '<i class="fas fa-pause"></i> Pause';
      btn.className = isPaused ? 'btn btn-success' : 'btn btn-warning';
      
      updateStatus(isPaused ? "System paused" : "System resumed");
      
      if (!isPaused && cart.length > 0 && cart[0].status === 'processing') {
        updateStatus("Resuming order processing...");
        processNextItem();
      }
    }

    function sendCommand(cmd) {
      fetch("/cmd?command=" + cmd);
    }

    function updateStatus(msg) {
      document.getElementById("statusText").innerText = msg;
    }

    // Initialize medicine cards
    document.addEventListener('DOMContentLoaded', function() {
      const container = document.getElementById('medicineContainer');
      
      medicines.forEach(med => {
        const progress = Math.min(100, (med.stock / 50) * 100);
        
        const card = document.createElement('div');
        card.className = 'medicine-card';
        card.innerHTML = `
          <div class="medicine-header">
            <div class="medicine-icon">
              <i class="fas ${med.icon}"></i>
            </div>
            <div>
              <div class="medicine-name">${med.name}</div>
              <div class="medicine-shelf">Shelf ${med.shelf}</div>
            </div>
          </div>
          <div class="medicine-body">
            <div class="medicine-stock">
              <span>In stock:</span>
              <span class="stock-count">${med.stock}</span>
            </div>
            <div class="stock-bar">
              <div class="stock-progress" style="width: ${progress}%"></div>
            </div>
            <div class="medicine-controls">
              <select class="quantity-selector" id="qty-${med.id}">
                ${Array.from({length: Math.min(5, med.stock)}, (_, i) => 
                  `
<option value="${i+1}">${i+1}</option>`).join('')}
              </select>
              <button class="add-btn" id="add-btn-${med.id}" onclick="addToCart(${med.id})">
                <i class="fas fa-plus"></i> Add
              </button>
            </div>
          </div>`;
        container.appendChild(card);
      });
    });
  </script>
</body>
</html>)rawliteral";
    server.send(200, "text/html", html);
  });

  server.on("/cmd", HTTP_GET, []() {
    String command = server.arg("command");
    arduinoSerial.println(command);
    server.send(200, "text/plain", "Command sent");
  });

  server.begin();
}

void loop() {
  server.handleClient();
  if (arduinoSerial.available()) {
    String response = arduinoSerial.readStringUntil('\n');
    Serial.println("Arduino: " + response);
  }
}






