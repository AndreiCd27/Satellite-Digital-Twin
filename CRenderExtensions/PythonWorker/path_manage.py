import os
import sys

def configure_paths(cpp_script_dir):
    # Insert the script's root directory at the highest priority position
    if cpp_script_dir not in sys.path:
        sys.path.insert(0, cpp_script_dir)

    # Reconstruct the site-packages paths
    explicit_site_packages = []

    for path in list(sys.path):
        if not path:
            continue
        if path.endswith("Lib") or path.endswith("lib"):
            sp_path = os.path.join(path, "site-packages")
            if os.path.exists(sp_path) and sp_path not in explicit_site_packages:
                explicit_site_packages.append(sp_path)
    
        parent_dir = os.path.dirname(path)
        sp_path_parallel = os.path.join(parent_dir, "site-packages")
        if os.path.exists(sp_path_parallel) and sp_path_parallel not in explicit_site_packages:
            explicit_site_packages.append(sp_path_parallel)

    try:
        import site
        if hasattr(site, 'getusersitepackages'):
            user_sp = site.getusersitepackages()
            if os.path.exists(user_sp) and user_sp not in explicit_site_packages:
                explicit_site_packages.append(user_sp)
    except Exception:
        pass

    # Inject all discovered site-packages into sys.path
    for sp in explicit_site_packages:
        if sp not in sys.path:
            sys.path.append(sp)