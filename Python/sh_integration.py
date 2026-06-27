import numpy as np
import scipy.integrate as integrate
import scipy.special as special

# Define the wedge size (e.g., 45 degrees in radians)
delta_phi = np.pi / 4.0 

def sh_norm(l, m):
    if m == 0:
        return np.sqrt((2*l + 1) / (4 * np.pi))
    else:
        return np.sqrt((2*l + 1) * special.factorial(l - abs(m)) / (2 * np.pi * special.factorial(l + abs(m))))

def K_m(m, phi):
    if m >= 0:
        return np.cos(m * phi)
    else:
        return np.sin(abs(m) * phi)

# 2D SH mapping expanded to L=3 (16 target coefficients)
sh_indices = [
    (0,0), 
    (1,-1), (1,0), (1,1), 
    (2,-2), (2,-1), (2,0), (2,1), (2,2),
    (3,-3), (3,-2), (3,-1), (3,0), (3,1), (3,2), (3,3)
]

# Setup the 16x8 target matrix
M_lin = np.zeros((16, 8))

for i in range(16):
    l, m = sh_indices[i]
    
    for j in range(8):
        # Splitting columns: first 4 belong to v_i, next 4 belong to v_i+1
        if j < 4:
            l_j = j
            f_phi = lambda phi: 1.0 - (phi / delta_phi)
        else:
            l_j = j - 4
            f_phi = lambda phi: phi / delta_phi
            
        # 1. Compute the Theta Integral
        norm_P_lj = np.sqrt((2 * l_j + 1) / 2.0)
        integrand_theta = lambda theta: (
            norm_P_lj * special.eval_legendre(l_j, np.cos(theta)) * 
            sh_norm(l, m) * special.lpmv(abs(m), l, np.cos(theta)) * np.sin(theta)
        )
        theta_integral, _ = integrate.quad(integrand_theta, 0, np.pi)
        
        # 2. Compute the Phi Integral
        integrand_phi = lambda phi: f_phi(phi) * K_m(m, phi)
        phi_integral, _ = integrate.quad(integrand_phi, 0, delta_phi)
        
        M_lin[i, j] = theta_integral * phi_integral

print("Flat array output for Engine usage (128 floats):")
print(", ".join([f"{val:.6f}f" for val in M_lin.flatten()]))
