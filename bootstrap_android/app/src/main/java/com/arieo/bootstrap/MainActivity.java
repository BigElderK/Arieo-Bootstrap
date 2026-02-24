package com.arieo.bootstrap;

import com.google.androidgamesdk.GameActivity;
import android.os.Bundle;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Environment;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import android.app.Application;
import java.lang.reflect.Method;

public class MainActivity extends GameActivity 
{
    private static final String TAG = "ArieoEngine";
    
    // Native method to set environment variable
    public native boolean nativeSetEnvironmentVariable(String name, String value);
    
    static 
    {
        // System.loadLibrary("arieo_main_module");
        
        try {
            // Get application context using reflection to call getFilesDir()
            Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
            Method currentApplicationMethod = activityThreadClass.getMethod("currentApplication");
            Application application = (Application) currentApplicationMethod.invoke(null);
            
            if (application != null) {
                File filesDir = application.getFilesDir();

                //Root path: /data/user/0/... vs /data/data/... (these are actually the same directory on Android — /data/data is a symlink to /data/user/0)
                // Deploy: /data/data/com.arieo.bootstrap/files
                // Runtime: /data/user/0/com.arieo.bootstrap/files
                String libCXXPath = filesDir.getAbsolutePath() + "/engine/Arieo-Bootstrap/android.armv8/bin/Release/libc++_shared.so";
                System.load(libCXXPath);
                Log.d("ArieoEngine", "Loaded libc++_shared.so from files directory: " + libCXXPath);

                String mainModulePath = filesDir.getAbsolutePath() + "/engine/modules/libarieo_main_module.so";
                System.load(mainModulePath);
                Log.d("ArieoEngine", "Loaded arieo_main_module from files directory: " + mainModulePath);
            } else {
                throw new RuntimeException("Could not get application context");
            }
        } catch (Exception e) {
            // Fallback: Load our main module from APK
            System.loadLibrary("arieo_main_module");
            Log.d("ArieoEngine", "Fallback: Loaded arieo_main_module from APK due to: " + e.getMessage());
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) 
    {
        super.onCreate(savedInstanceState);
        
        // Log which library loading method was used
        logLibraryLoadingMethod();
        
        // Load arieo.application.manifest from manifest meta-data
        Log.d(TAG, "MainActivity onCreate() called");
        
        String applicationManifest = getArieoApplicationManifest();
        Log.d(TAG, "Raw application manifest: " + applicationManifest);
        
        String obbFolder = getObbFolder();
        Log.d(TAG, "OBB folder: " + obbFolder);
        
        // Set app internal data directory as environment variable - using internal files dir to avoid namespace issues
        String appInternalDataDir = getFilesDir().getAbsolutePath();
        Log.d(TAG, "Setting APP_INTERNAL_DATA_DIR environment variable to: " + appInternalDataDir);
        try {
            boolean success = nativeSetEnvironmentVariable("APP_INTERNAL_DATA_DIR", appInternalDataDir);
            if (success) {
                Log.d(TAG, "Successfully set APP_DATA_DIR environment variable via JNI");
            } else {
                Log.e(TAG, "JNI call failed to set APP_INTERNAL_DATA_DIR environment variable");
            }
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "JNI method nativeSetEnvironmentVariable not found", e);
        }

        // Set app external data directory as environment variable - using internal files dir to avoid namespace issues
        String appExternalDataDir = getExternalFilesDir(null).getAbsolutePath();
        Log.d(TAG, "Setting APP_EXTERNAL_DATA_DIR environment variable to: " + appExternalDataDir);
        try {
            boolean success = nativeSetEnvironmentVariable("APP_EXTERNAL_DATA_DIR", appExternalDataDir);
            if (success) {
                Log.d(TAG, "Successfully set APP_EXTERNAL_DATA_DIR environment variable via JNI");
            } else {
                Log.e(TAG, "JNI call failed to set APP_EXTERNAL_DATA_DIR environment variable");
            }
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "JNI method nativeSetEnvironmentVariable not found", e);
        }

        // Replace the placeholder with actual OBB folder path:
        String manifestValue = applicationManifest; // Gets "${obb_folder}/arieo_render_test_app"
        if (manifestValue != null && manifestValue.contains("$${obb_folder}")) {
            manifestValue = manifestValue.replace("$${obb_folder}", obbFolder);
            Log.d(TAG, "Replaced manifest value: " + manifestValue);
        }

        // Set real system environment variable using native JNI call
        if (manifestValue != null) {
            Log.d(TAG, "Setting APP_MANIFEST_PATH environment variable to: " + manifestValue);
            
            try {
                boolean success = nativeSetEnvironmentVariable("APP_MANIFEST_PATH", manifestValue);
                
                if (success) {
                    Log.d(TAG, "Successfully set APP_MANIFEST_PATH environment variable via JNI");
                    // Verify by reading it back from system
                    String verifyValue = System.getenv("APP_MANIFEST_PATH");
                    Log.d(TAG, "Verification - APP_MANIFEST_PATH = " + verifyValue);
                } else {
                    Log.e(TAG, "JNI call failed to set APP_MANIFEST_PATH environment variable");
                }
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "JNI method nativeSetEnvironmentVariable not found", e);
            }
        } else {
            Log.e(TAG, "arieo.application.manifest meta-data not found or null");
        }


    }
    
    private String getArieoApplicationManifest() {
        try {
            ApplicationInfo appInfo = getPackageManager().getApplicationInfo(
                getPackageName(), PackageManager.GET_META_DATA);
            
            if (appInfo.metaData != null) {
                return appInfo.metaData.getString("arieo.application.manifest");
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Package name not found", e);
        } catch (Exception e) {
            Log.e(TAG, "Error reading meta-data", e);
        }
        return null;
    }
    
    private String getObbFolder() {
        try {
            // Get the main OBB directory for this app
            File obbDir = getObbDir();
            if (obbDir != null && obbDir.exists()) {
                // Return the OBB directory path
                String obbDirPath = obbDir.getAbsolutePath();
                Log.d(TAG, "Found OBB directory: " + obbDirPath);
                return obbDirPath;
            }
            
            // Fallback: construct expected OBB folder path
            File externalStorage = Environment.getExternalStorageDirectory();
            File obbFolder = new File(externalStorage, "Android/obb/" + getPackageName());
            
            // Return the OBB folder path (create if doesn't exist)
            String obbFolderPath = obbFolder.getAbsolutePath();
            Log.d(TAG, "Using OBB folder path: " + obbFolderPath);
            return obbFolderPath;
            
        } catch (Exception e) {
            Log.e(TAG, "Error getting OBB folder path", e);
            return null;
        }
    }
    
    private void logLibraryLoadingMethod() {
        try {
            // Check if library was loaded from internal files directory
            String internalLibPath = getFilesDir().getAbsolutePath() + "/lib/libarieo_main_module.so";
            File internalLib = new File(internalLibPath);
            
            if (internalLib.exists() && internalLib.canRead()) {
                Log.d(TAG, "arieo_main_module loaded from internal files directory: " + internalLibPath);
                Log.d(TAG, "Internal library size: " + internalLib.length() + " bytes");
            } else {
                Log.d(TAG, "arieo_main_module loaded from APK (internal files version not available)");
                if (internalLib.exists()) {
                    Log.w(TAG, "Internal library exists but not readable: " + internalLibPath);
                } else {
                    Log.d(TAG, "Internal library does not exist: " + internalLibPath);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error checking library loading method", e);
        }
    }
}