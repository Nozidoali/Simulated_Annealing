import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('iter.csv')
df['iter'] = range( len(df['size']) )
ax = df.plot( x = 'iter', y = 'size', title = 'size over iteration' )
plt.show()
