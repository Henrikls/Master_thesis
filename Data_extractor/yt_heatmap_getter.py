import os
import time
from selenium import webdriver
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.firefox.firefox_profile import FirefoxProfile

# I should probably make an init method, ...

def extract_svgs_from_youtube(video_url):
    """Extracts heatmaps by injecting JavaScript into the YouTube page using Selenium."""
    
    # Update this path to point to where you want the files to be downloaded
    SVG_SAVE_DIR = "/mnt/c/Users/Henri/Desktop/Mastersprojekt/data_gather/svgs"
    if not os.path.exists(SVG_SAVE_DIR):
        os.makedirs(SVG_SAVE_DIR)

    # JavaScript code to extract and download heatmaps (the same one you tested in the console)
    js_code = """
    (function() {
        // Select all the heatmap SVG elements
        const heatmaps = document.querySelectorAll('.ytp-heat-map-container svg');

        heatmaps.forEach((heatmap, index) => {
            // Remove any previously set width and height to allow SVG to scale based on its own aspect ratio
            heatmap.removeAttribute('width');
            heatmap.removeAttribute('height');
            
            // Ensure the viewBox is preserved (if not already set in the SVG)
            if (!heatmap.hasAttribute('viewBox')) {
                const bbox = heatmap.getBBox();  // Get the bounding box of the SVG content
                heatmap.setAttribute('viewBox', `0 0 ${bbox.width} ${bbox.height}`);
            }

            // Serialize each heatmap SVG to a string
            const svgData = new XMLSerializer().serializeToString(heatmap);

            // Create a Blob from the SVG data
            const svgBlob = new Blob([svgData], { type: 'image/svg+xml;charset=utf-8' });
            const url = URL.createObjectURL(svgBlob);

            // Create a download link and download the SVG file
            const downloadLink = document.createElement('a');
            downloadLink.href = url;
            downloadLink.download = `heatmap_${index + 1}.svg`;  // Unique filename for each heatmap
            document.body.appendChild(downloadLink);
            downloadLink.click();
            document.body.removeChild(downloadLink);

            // Revoke the object URL after downloading
            URL.revokeObjectURL(url);
        });
    })();
    """
    
    options = Options()
    # Set up Firefox profile to change the default download directory
    profile = FirefoxProfile()
    profile.set_preference("browser.download.folderList", 2)  # Use custom download directory
    profile.set_preference("browser.download.dir", SVG_SAVE_DIR)  # Set download directory to the svg folder
    profile.set_preference("browser.helperApps.neverAsk.saveToDisk", "image/svg+xml")  # Automatically download SVGs without asking
    options.profile = profile
    # Set up Firefox in headless mode
    options.add_argument("--headless")  # Headless mode
    
    # Initialize Firefox WebDriver (geckodriver should be in your PATH)
    driver = webdriver.Firefox(options=options)

    try:
        # Open the YouTube video
        driver.get(video_url)
        time.sleep(5)  # Wait for the page to load completely

        # Scroll down the page to ensure heatmaps are loaded
        driver.execute_script("window.scrollTo(0, document.body.scrollHeight);")
        time.sleep(3)

        # Inject the JavaScript code into the page
        driver.execute_script(js_code)
        time.sleep(5)  # Allow time for the SVGs to be downloaded

        print("SVG heatmaps downloaded successfully to {SVG_SAVE_DIR}.")

    finally:
        # Close the browser
        driver.quit()