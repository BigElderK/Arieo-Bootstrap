/**
 * Arieo Engine Manifest Handler
 * Handles extraction and processing of app.manifest.yaml from OBB files
 */

class ManifestHandler {
    constructor() {
        this.manifestPath = null;
        this.manifestContent = null;
    }

    /**
     * Main function to setup manifest from OBB
     * This replaces the C++ setupManifestFromObb function
     */
    async setupManifestFromObb() {
        try {
            console.log('Starting JavaScript manifest setup from OBB');
            
            // Step 1: Get application metadata from AndroidManifest.xml
            const appName = await this.getApplicationMetadata('arieo.application.name');
            if (!appName) {
                console.error('Failed to get arieo.application.name from metadata');
                return false;
            }
            console.log('Application name:', appName);

            // Step 2: Get OBB directory path
            const obbDir = await this.getObbFilePath();
            if (!obbDir) {
                console.error('Failed to get OBB directory path');
                return false;
            }
            console.log('OBB directory:', obbDir);

            // Step 3: Construct target file path within OBB
            const targetFile = `applications/${appName}/app.manifest.yaml`;
            console.log('Target file in OBB:', targetFile);

            // Step 4: Extract manifest content from OBB
            const manifestContent = await this.extractFileFromObb(obbDir, targetFile);
            if (!manifestContent) {
                console.warn('Failed to extract manifest from OBB, creating dummy content');
                return this.createDummyManifest(appName);
            }

            // Step 5: Create temporary file with manifest content
            const tempFilePath = await this.createTempManifestFile(manifestContent, appName);
            if (!tempFilePath) {
                console.error('Failed to create temporary manifest file');
                return false;
            }

            // Step 6: Set environment variable
            const success = await this.setEnvironmentVariable('ARIEO_APP_MANIFEST_PATH', tempFilePath);
            if (!success) {
                console.error('Failed to set environment variable ARIEO_APP_MANIFEST_PATH');
                return false;
            }

            console.log('Successfully set ARIEO_APP_MANIFEST_PATH=' + tempFilePath);
            this.manifestPath = tempFilePath;
            this.manifestContent = manifestContent;
            
            // Step 7: Log the manifest content for debugging
            this.logManifestContent(manifestContent);
            
            return true;

        } catch (error) {
            console.error('Error in setupManifestFromObb:', error);
            return false;
        }
    }

    /**
     * Get application metadata from AndroidManifest.xml via JNI
     */
    async getApplicationMetadata(metaDataName) {
        return new Promise((resolve, reject) => {
            try {
                // Call native method via JNI bridge
                const result = window.ArieoNative.getApplicationMetadata(metaDataName);
                resolve(result);
            } catch (error) {
                console.error('Error getting application metadata:', error);
                resolve(null);
            }
        });
    }

    /**
     * Get OBB file path via JNI
     */
    async getObbFilePath() {
        return new Promise((resolve, reject) => {
            try {
                const result = window.ArieoNative.getObbFilePath();
                resolve(result);
            } catch (error) {
                console.error('Error getting OBB file path:', error);
                resolve(null);
            }
        });
    }

    /**
     * Extract file from OBB via JNI
     */
    async extractFileFromObb(obbDir, targetFile) {
        return new Promise((resolve, reject) => {
            try {
                const result = window.ArieoNative.extractFileFromObb(obbDir, targetFile);
                resolve(result);
            } catch (error) {
                console.error('Error extracting file from OBB:', error);
                resolve(null);
            }
        });
    }

    /**
     * Create temporary manifest file via JNI
     */
    async createTempManifestFile(content, appName) {
        return new Promise((resolve, reject) => {
            try {
                const tempDir = '/data/data/com.arieo.bootstrap/cache/';
                const tempFileName = `app_manifest_${appName}.yaml`;
                const result = window.ArieoNative.createTempFile(tempDir, tempFileName, content);
                resolve(result);
            } catch (error) {
                console.error('Error creating temp manifest file:', error);
                resolve(null);
            }
        });
    }

    /**
     * Set environment variable via JNI
     */
    async setEnvironmentVariable(name, value) {
        return new Promise((resolve, reject) => {
            try {
                const result = window.ArieoNative.setEnvironmentVariable(name, value);
                resolve(result);
            } catch (error) {
                console.error('Error setting environment variable:', error);
                resolve(false);
            }
        });
    }

    /**
     * Create dummy manifest content when extraction fails
     */
    async createDummyManifest(appName) {
        try {
            const dummyContent = `# App Manifest (dummy - extraction failed)
name: "${appName}"
version: "1.0.0"
note: "Actual OBB extraction failed, using dummy content"
generated_by: "JavaScript ManifestHandler"
timestamp: "${new Date().toISOString()}"
`;

            const tempFilePath = await this.createTempManifestFile(dummyContent, appName);
            if (tempFilePath) {
                await this.setEnvironmentVariable('ARIEO_APP_MANIFEST_PATH', tempFilePath);
                console.log('Created dummy manifest at:', tempFilePath);
                return true;
            }
            return false;
        } catch (error) {
            console.error('Error creating dummy manifest:', error);
            return false;
        }
    }

    /**
     * Log manifest content for debugging
     */
    logManifestContent(content) {
        if (!content) return;
        
        console.log('=== MANIFEST CONTENT START ===');
        const lines = content.split('\n');
        lines.forEach((line, index) => {
            console.log(`MANIFEST: ${line}`);
        });
        console.log('=== MANIFEST CONTENT END ===');
    }

    /**
     * Get the current manifest path
     */
    getManifestPath() {
        return this.manifestPath;
    }

    /**
     * Get the current manifest content
     */
    getManifestContent() {
        return this.manifestContent;
    }
}

// Initialize the manifest handler
const manifestHandler = new ManifestHandler();

// Expose to global scope for native code to call
window.ArieoManifestHandler = manifestHandler;

// Export for module usage
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ManifestHandler;
}