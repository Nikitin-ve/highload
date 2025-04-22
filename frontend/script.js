// GET Request
document.getElementById('getButton').addEventListener('click', async function() {
    const key = document.getElementById('getKey').value;
    const resultDiv = document.getElementById('getResult');

    if (!key) {
        resultDiv.textContent = 'Please enter a key';
        return;
    }

    try {
        const response = await fetch(`/v1/key-value?key=${encodeURIComponent(key)}`, {
            method: 'GET',
            headers: { 'Content-Type': 'application/json' }
        });
        
        const data = await response.text();

        if (response.ok) {
            resultDiv.textContent = `Value: ${data}`;
        } else {
            resultDiv.textContent = data || 'No such key';
        }
    } catch (err) {
        resultDiv.textContent = 'Connection error';
    }
});

// POST Request
document.getElementById('postButton').addEventListener('click', async function() {
    const key = document.getElementById('postKey').value;
    const value = document.getElementById('postValue').value;
    const resultDiv = document.getElementById('postResult');

    if (!key || !value) {
        resultDiv.textContent = 'Please enter both key and value';
        return;
    }

    try {
        /*const response = await fetch('/v1/key-value', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ key, value })
        });*/
        const response = await fetch(`/v1/key-value?key=${encodeURIComponent(key)}&value=${encodeURIComponent(value)}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' }
        });

        const data = await response.text();
        console.log(data);

        if (response.ok) {
            resultDiv.textContent = `Value "${data}" set on key "${key}"`;
        } else {
            resultDiv.textContent = data.error || 'Error setting value';
        }
    } catch (err) {
        resultDiv.textContent = 'Connection error';
    }
});

// DELETE Request
document.getElementById('deleteButton').addEventListener('click', async function() {
    const key = document.getElementById('deleteKey').value;
    const resultDiv = document.getElementById('deleteResult');

    if (!key) {
        resultDiv.textContent = 'Please enter a key';
        return;
    }

    try {
        const response = await fetch(`/v1/key-value?key=${encodeURIComponent(key)}`, {
            method: 'DELETE',
            headers: { 'Content-Type': 'application/json' }
        });

        const data = await response.text();

        if (response.ok) {
            if (data === '0') {
                resultDiv.textContent = `No such key. ${data} items deleted.`;
            } else {
                resultDiv.textContent = `Number of deleted items: ${data}`;
            }
        } else {
            resultDiv.textContent = data.error || 'Error deleting key';
        }
    } catch (err) {
        resultDiv.textContent = 'Connection error';
    }
});

// Prevent form submissions
document.querySelectorAll('.key-value-form').forEach(form => {
    form.addEventListener('submit', function(e) {
        e.preventDefault();
    });
});