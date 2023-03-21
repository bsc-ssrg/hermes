# Build stage with Spack pre-installed and ready to be used
FROM spack/ubuntu-bionic:latest as builder


# What we want to install and how we want to install it
# is specified in a manifest file (spack.yaml)
RUN mkdir /opt/spack-environment \
&&  (echo "spack:" \
&&   echo "  specs:" \
&&   echo "  - mercury@2.2.0" \
&&   echo "  - mercury@2.1.0" \
&&   echo "  - mercury@2.0.1" \
&&   echo "  - mercury@2.0.0" \
&&   echo "  - mercury@1.0.1" \
&&   echo "  - mercury@1.0.0" \
&&   echo "  - mercury@0.9.0" \
&&   echo "  - margo@0.9.1" \
&&   echo "  - margo@0.9" \
&&   echo "  - margo@0.7.2" \
&&   echo "  - margo@0.7.1" \
&&   echo "  - margo@0.7" \
&&   echo "  - margo@0.6.4" \
&&   echo "  - margo@0.6.3" \
&&   echo "  - margo@0.6.2" \
&&   echo "  - margo@0.6.1" \
&&   echo "  - margo@0.6" \
&&   echo "  - margo@0.5.2" \
&&   echo "  - margo@0.5.1" \
&&   echo "  - margo@0.5" \
&&   echo "  - margo@0.4.7" \
&&   echo "  - margo@0.4.6" \
&&   echo "  - margo@0.4.5" \
&&   echo "  - margo@0.4.4" \
&&   echo "  - margo@0.4.3" \
&&   echo "  - margo@0.4.2" \
&&   echo "  concretizer:" \
&&   echo "    unify: when_possible" \
&&   echo "  config:" \
&&   echo "    install_tree: /opt/software" \
&&   echo "  view: /opt/view") > /opt/spack-environment/spack.yaml

# Install the software, remove unnecessary deps
RUN cd /opt/spack-environment && spack env activate . && spack install --fail-fast && spack gc -y

# Strip all the binaries
RUN find -L /opt/view/* -type f -exec readlink -f '{}' \; | \
    xargs file -i | \
    grep 'charset=binary' | \
    grep 'x-executable\|x-archive\|x-sharedlib' | \
    awk -F: '{print $1}' | xargs strip -s

# Modifications to the environment that are necessary to run
RUN cd /opt/spack-environment && \
    spack env activate --sh -d . >> /etc/profile.d/z10_spack_environment.sh

# Bare OS image to run the installed executables
FROM ubuntu:18.04

COPY --from=builder /opt/spack-environment /opt/spack-environment
COPY --from=builder /opt/software /opt/software
COPY --from=builder /opt/._view /opt/._view
COPY --from=builder /opt/view /opt/view
COPY --from=builder /etc/profile.d/z10_spack_environment.sh /etc/profile.d/z10_spack_environment.sh

ENTRYPOINT ["/bin/bash", "--rcfile", "/etc/profile", "-l", "-c", "$*", "--" ]
CMD [ "/bin/bash" ]

