#!/usr/bin/env python
# coding: utf-8

# In[1]:


# Packages / libraries
import os #provides functions for interacting with the operating system
import numpy as np 
import pandas as pd

# for visualizing 
from matplotlib import pyplot as plt
import seaborn as sns

get_ipython().run_line_magic('matplotlib', 'inline')

# To change scientific numbers to float
np.set_printoptions(formatter={'float_kind':'{:f}'.format})

# Increases the size of sns plots
sns.set(rc={'figure.figsize':(50,50)})

# to split, built model, and evaluate it

from sklearn.preprocessing import MinMaxScaler
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, confusion_matrix, r2_score, classification_report


# In[2]:


#################################################################################################################
################################ Getting the file from local CSV      ###########################################
#################################################################################################################

# Loading the data
raw_data = pd.read_csv('actual_data.csv', encoding='latin-1')

# print the shape
print(raw_data.shape)

# see data
raw_data


# In[3]:


# Your code goes here
X = raw_data.drop('Target', axis=1).values# Input features (attributes)
y = raw_data['Target'].values # Target vector
print('X shape: {}'.format(np.shape(X)))
print('y shape: {}'.format(np.shape(y)))

X_train, X_test, y_train, y_test = train_test_split(X, y, train_size = 0.8, test_size=0.2, random_state=0)


# In[4]:


# Confusion Matrix function

def plot_confusion_matrix(cm, classes=None, title='Confusion matrix'):
    """Plots a confusion matrix."""
    if classes is not None:
        sns.heatmap(cm, xticklabels=classes, yticklabels=classes, vmin=0., vmax=1., annot=True, annot_kws={'size':50})
    else:
        sns.heatmap(cm, vmin=0., vmax=1.)
    plt.title(title)
    plt.ylabel('True label')
    plt.xlabel('Predicted label')


# In[5]:



rf = RandomForestClassifier(max_features = 1, max_depth = 4, n_estimators = 193, min_samples_split = 2,
                            min_samples_leaf = 1, bootstrap = False, criterion='entropy', ccp_alpha=0.0001,
                            random_state=0)
rf.fit(X_train, y_train)
prediction_test = rf.predict(X=X_test)


# Accuracy on Train
print("Training Accuracy is: ", rf.score(X_train, y_train))

# Accuracy on Test
print("Testing Accuracy is: ", rf.score(X_test, y_test))

# Confusion Matrix
cm = confusion_matrix(y_test, prediction_test)
cm_norm = cm/cm.sum(axis=1)[:, np.newaxis]
plt.figure()
plot_confusion_matrix(cm_norm, classes = rf.classes_)


# In[10]:


print(classification_report(y_test, prediction_test))


# In[96]:


# PH -> 5.5-6.5 & PPM -> 560-840 & TEMP -> 20-25
rf.predict([[5.51,840.01,19.99]])


# In[101]:


rf.predict([[5.91,845.01,18.99]])


# In[109]:


rf.predict([[5.91,745.01,18.99]])


# In[111]:


rf.predict([[5.91,1000,0]])


# In[15]:


# normal PH , High PPM , Low TEMP
for PH in np.arange(start = 5.5, stop = 6.501, step = 0.1):
    for PPM in np.arange(start = 840.1, stop = 1000, step = 1):
        for TEMP_day in np.arange(start = 15, stop = 19.99, step = 1):
            X = rf.predict([[PH,PPM,TEMP_day]])
            print(X)
            if (X == [11]):
                print("yes")
            else:
                break


# In[16]:


import pickle

with open('model_pickle','wb') as f:
    pickle.dump(rf,f)


# In[17]:


with open('model_pickle','rb') as f:
    mod = pickle.load(f)


# In[18]:


# normal PH , High PPM , Low TEMP
for PH in np.arange(start = 5.5, stop = 6.501, step = 0.1):
    for PPM in np.arange(start = 840.1, stop = 1000, step = 1):
        for TEMP_day in np.arange(start = 15, stop = 19.99, step = 1):
            X = mod.predict([[PH,PPM,TEMP_day]])
            print(X)
            if (X == [11]):
                print("yes")
            else:
                break


# In[ ]:


import serial
import numpy as np
import pandas as pd
import pickle

with open('model_pickle','rb') as f:
    mod = pickle.load(f)

if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0',9600, timeout=1)
    ser.flush() # Waits for the transmission of outgoing serial data to complete. 

list_recieved = [] # outside infinte loop
E = 0

while True:
    if ser.in_waiting > 0: # it means if something is transmitted from arduino
        line = ser.readlin().decode('utf-8').rstrip() # we recieve elements in string format and The rstrip() method removes
                                                      # any trailing characters (characters at the end a string)
        if len(line) < 7:
            line = round(float(line),2)
            list_recieved.append(line)
        else:
            list_recieved.append(line)
            
    else if ser.in_waiting == 0:
        if len(list_recieved) == 7:
            # check on the lenght of array if it's equal to 6 then continue on checking on the missing data as follows
            # array [date, PH, PPM, Air_Temp, water_level, Humd, Water_temp]
            #list_recieved = ['2022-6-22',5.6,50,19,358,30,22]
            x_1 = pd.DataFrame(list_recieved)
            
            if x_1[0][1] == -1:
                print("something wrong with PH sensor")
                E = -1
                print("see how to send this value to cloud and arduino")
                
            if x_1[0][2] == -2:
                print("something wrong with TDS sensor")
                E = -2
                print("see how to send this value to cloud and arduino")
                
            if x_1[0][3] == -3:
                print("something wrong with Air temperature (DHT22) sensor")
                E = -3
                print("see how to send this value to cloud and arduino")
                
            if x_1[0][4] == -4:
                print("something wrong with Water level sensor")
                E = -4
                print("see how to send this value to cloud and arduino")
                
            if x_1[0][5] == -5:
                print("something wrong with DHT22 sensor (humidity)")
                E = -5
                print("see how to send this value to cloud and arduino")
                
            if x_1[0][6] == -6:
                print("something wrong with Water temperature sensor")
                E = -6
                print("see how to send this value to cloud and arduino")
                
            if (x_1[0][1] != -1) & (x_1[0][2] != -2) & (x_1[0][3] != -3) & (x_1[0][4] != -4) &            (x_1[0][5] != -5) & (x_1[0][6] != -6) &            (pd.DataFrame(x_1.isnull().any()).equals(pd.DataFrame([False])) == True): 
                y= np.transpose(x_1[1:4])
                print(y)
                pred_RF_3 = mod.predict(y.values)
                print(int(pred_RF_3))
                print("now send this Label to Arduino to process it and come up with decisions")
                ser.write(str(pred_RF_3).encode('utf-8'))

        else if (len(list_recieved) > 7) & (E == 0):
     # means label or prediction sent back to Arduino and after doing control decisions on different actuators. It sends 
     # decisions in string format such as "turn on cooler" and sent it back to raspberry pi and store it in list   
     # called "list_recieved" one by one as it is going to check lenght of list if it's higher 7 means it is now time 
     # to send array to cloud.
            print("see how to send this array to cloud")
            # clear all to store new data coming from ardiuno
            list_recieved.clear()
            x_1  = x_1 [0:0]
            y = y[0:0] 
            E = 0
        
        else:
            pass
    else:
        pass


# In[ ]:




